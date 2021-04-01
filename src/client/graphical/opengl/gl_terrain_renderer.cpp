#include <client/graphical/opengl/gl_terrain_renderer.hpp>
#include <algorithm>
#include <cassert>
#include <client/graphical/TextureOpener.hpp>
#include <client/graphical/opengl/gl_renderer.hpp>
#include <client/graphical/gfx_service.hpp>
#include <cmath>
#include <common/logger.hpp>
#include <iterator>

#include <config.h>


using namespace familyline::graphics;
using namespace familyline::logic;

/***
    Renderer


    1    2    3    4    5
    +----+----+----+----+
    |    |    |    |    |
    |w+1 |w*2 |    |    |
    +----+----+----+----+
    |    |    |    |    |
    |2w+1|    |    |    |
    +----+----+----+----+
    |    |    |    |    |
    |    |    |    |    |
    +----+----+----+----+

***/
#include <cmath>

GLTerrainRenderer::GLTerrainRenderer(Camera& cam)
    : TerrainRenderer(cam)
{
    auto& d = GFXService::getDevice();

    sTerrain_ = d->createShaderProgram(
        "terrain", {d->createShader("shaders/Terrain.vert", ShaderType::Vertex),
                    d->createShader("shaders/Terrain.frag", ShaderType::Fragment)});

    sTerrain_->link();
}

std::vector<glm::vec3> GLTerrainRenderer::createNormals(
    const std::vector<glm::vec3>& vertices, int width) const
{
    auto [w, h] = terr_->getSize();
    std::vector<glm::vec3> normals(w * h, glm::vec3(0, 0, 0));

    /* Calculate the normals
       Calculate the normal of every triangle that is part of a single vertex and sum them
         /|\           The vertex we need to calculate the normal is the 'O'.
        / | \          If possible, we need to get the normals of all four triangles there.
       /  |  \
      *---O---*        This might mean that no rough edges will be possible, but a "cliff" terrain
       \  |  /         type will exist (like in AoE2)
        \ | /
         \|/
    */

    for (auto y = 0; y < h; y++) {
        for (auto x = 0; x < w; x++) {
            const auto idx = (y * w + x);

            glm::vec3 norms[4];
            int q = 0;

            norms[q++] = vertices[idx];

            if (x < w - 1) {
                const auto v2idx = y * w + x + 1;
                norms[q++]       = vertices[v2idx];
            }

            if (x < w - 1 && y < h - 1) {
                const auto v3idx = (y + 1) * w + (x + 1);
                norms[q++]       = vertices[v3idx];
            }

            if (y < h - 1) {
                const auto v4idx = (y + 1) * w + x;
                norms[q++]       = vertices[v4idx];
            }

            auto vnormal = glm::vec3(0, 0, 0);
            for (auto i = 0; i < q; i++) {
                auto current = norms[i];
                auto next    = norms[(i + 1) % q];

                vnormal = glm::vec3(
                    vnormal.x + ((current.y - next.y) * (current.z + next.z)),
                    vnormal.y + ((current.z - next.z) * (current.x + next.x)),
                    vnormal.z + ((current.x - next.x) * (current.y + next.y)));
            }

            vnormal = glm::normalize(vnormal);

            if (std::isnan(vnormal.x) || std::isnan(vnormal.y))
                LoggerService::getLogger()->write(
                    "terrain-renderer", LogType::Error,
                    "normal of (%.3f, %.3f, %.3f) [  (%.3f, %.3f, %.3f) ]"
                    "gave NaN",
                    vertices[idx].x, vertices[idx].y, vertices[idx].z, vnormal.x, vnormal.y,
                    vnormal.z);

            normals[idx] = -vnormal;
        }
    }

    return normals;
}

/**
 * Create the indices.
 *
 * The indices make each of those squares go to the video card
 * in a clockwise order
 */
std::vector<unsigned int> GLTerrainRenderer::createIndices(
    const std::vector<glm::vec3>& vertices, int width) const
{
    auto height = vertices.size() / width;

    std::vector<unsigned int> indices;

    for (auto y = 0; y < height; y++) {
        for (auto x = 0; x < width; x++) {
            const int idx[4] = {
                y * width + x, ((x + 1) >= width) ? y * width + x : y * width + (x + 1),
                ((x + 1) >= width || (y + 1) >= height)
                    ? ((x + 1) >= width && (y + 1) < height)
                          ? (y + 1) * width + x
                          : ((x + 1) < width && (y + 1) >= height) ? y * width + (x + 1)
                                                                   : y * width + x
                    : (y + 1) * width + (x + 1),
                ((y + 1) >= height) ? y * width + x : (y + 1) * width + x};

            indices.push_back(idx[0]);
            indices.push_back(idx[1]);
            indices.push_back(idx[2]);

            indices.push_back(idx[0]);
            indices.push_back(idx[2]);
            indices.push_back(idx[3]);
        }
    }

    return indices;
}

TerrainRenderInfo GLTerrainRenderer::createTerrainData()
{
    TerrainRenderInfo tri;

    auto& tdata = terr_->getHeightData();
    auto [w, h] = terr_->getSize();

    for (auto y = 0; y < h; y++) {
        for (auto x = 0; x < w; x++) {
            auto idx    = y * w + x;
            auto height = tdata[idx];
            tri.vertices.push_back(terr_->gameToGraphical(glm::vec3(x, height, y)));
        }
    }

    tri.normals = this->createNormals(tri.vertices, w);
    tri.indices = this->createIndices(tri.vertices, w);

    return tri;
}

GLuint GLTerrainRenderer::createTerrainDataVAO()
{
    auto err = glGetError();

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    auto fnGetAttrib = [&](const char* name) {
        return glGetAttribLocation(sTerrain_->getHandle(), name);
    };

    GLuint vboVertices, vboNormals, vboTextures, vboTexIndices, vboElement;
    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(
        GL_ARRAY_BUFFER, tri_.vertices.size() * sizeof(glm::vec3), tri_.vertices.data(),
        GL_STATIC_DRAW);
    glVertexAttribPointer(fnGetAttrib("position"), 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(
        GL_ARRAY_BUFFER, tri_.normals.size() * sizeof(glm::vec3), tri_.normals.data(),
        GL_STATIC_DRAW);
    glVertexAttribPointer(fnGetAttrib("normal"), 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &vboTextures);
    glBindBuffer(GL_ARRAY_BUFFER, vboTextures);
    glBufferData(
        GL_ARRAY_BUFFER, tri_.texcoords.size() * sizeof(glm::vec2), tri_.texcoords.data(),
        GL_STATIC_DRAW);
    glVertexAttribPointer(fnGetAttrib("texcoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &vboTexIndices);
    glBindBuffer(GL_ARRAY_BUFFER, vboTexIndices);
    glBufferData(
        GL_ARRAY_BUFFER, tri_.texture_ids.size() * sizeof(unsigned int), tri_.texture_ids.data(),
        GL_STATIC_DRAW);
    glVertexAttribPointer(fnGetAttrib("texidx"), 1, GL_UNSIGNED_INT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(3);

    glGenBuffers(1, &vboElement);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboElement);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, tri_.indices.size() * sizeof(unsigned int), tri_.indices.data(),
        GL_STATIC_DRAW);

    auto& log = LoggerService::getLogger();
    err       = glGetError();
    if (err != GL_NO_ERROR) {
        log->write("terrain-renderer", LogType::Warning, "GL error %#x while creating VAO", err);
    }

    return vao;
}

// TODO: load only terrains that the terrain file uses
std::vector<TerrainTexInfo> loadTerrainData()
{
    return {
        {"grass", 16, 16},
        {"sand", 16, 16},
    };
}

std::unordered_map<TerrainType, unsigned int> loadTerrainTypes()
{
    return {{TerrainType::Grass, 0}, {TerrainType::Sand, 1}};
}

/**
 * Build the terrain textures, and get the terrain texcoords.
 *
 * The terrain texcoords will increase until the vertex point is equal to the scale,
 * decrease to 0 until it is scale*2, increase again to 1 until scale*3, and et cetera,
 * more or less like a sine wave.
 */
void GLTerrainRenderer::buildTextures()
{
    tatlas_ = TextureOpener::OpenTexture(TEXTURES_DIR "/terrain/texatlas.png");

    terrain_data_     = loadTerrainData();
    terr_type_to_idx_ = loadTerrainTypes();

    auto& typedata = terr_->getTypeData();
    auto [w, h]    = terr_->getSize();

    for (auto td : typedata) {
        TerrainType terraintype = (TerrainType)td;
        tri_.texture_ids.push_back(terr_type_to_idx_[terraintype]);
    }

    auto fnGetTexCoordFromPos = [&](double val, double scale) {
        return 0.5 + ((M_PI / 10) * asin(sin(((2 * M_PI) / (2 * scale)) * (val - (scale / 2)))));
    };

    auto idx = 0;
    for (auto tpos : tri_.vertices) {
        auto terr_idx         = tri_.texture_ids[idx];
        TerrainTexInfo& tinfo = terrain_data_[terr_idx];

        tri_.texcoords.push_back(glm::vec2(
            fnGetTexCoordFromPos(tpos.x, tinfo.xscale),
            fnGetTexCoordFromPos(tpos.z, tinfo.yscale)));

        idx++;
    }

    auto& log = LoggerService::getLogger();
    log->write("terrain-renderer", LogType::Info, "Terrain textures built");

    tvao_ = this->createTerrainDataVAO();
}

void GLTerrainRenderer::buildVertexData() { tri_ = this->createTerrainData(); }

const int slot_texture_size = 16;

/**
 * Render the terrain
 *
 * We use the GL renderer to let it render the lights itself, and
 * so we do not duplicate code to render lights here
 */
void GLTerrainRenderer::render(Renderer& r)
{
    assert(terr_ != nullptr);
    GLRenderer& rnd = dynamic_cast<GLRenderer&>(r);

    auto& log = LoggerService::getLogger();
    auto err  = glGetError();

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // we're not using the stencil buffer now

    auto& sm = GFXService::getShaderManager();
    sm->use(*sTerrain_);

    sTerrain_->setUniform("mWorld", glm::mat4(1.0));
    sTerrain_->setUniform("mView", cam_.GetViewMatrix());
    sTerrain_->setUniform("mProjection", cam_.GetProjectionMatrix());
    sTerrain_->setUniform("diffuse_color", glm::vec3(0.5, 0.5, 0.5));
    sTerrain_->setUniform("ambient_color", glm::vec3(0.1, 0.1, 0.1));

    rnd.drawLights(*sTerrain_);

    sTerrain_->setUniform("tex_amount", 1.0f);

    glBindVertexArray(tvao_);

    glBindTexture(GL_TEXTURE_2D, tatlas_->GetHandle());
    glDrawElements(GL_TRIANGLES, tri_.indices.size(), GL_UNSIGNED_INT, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    err = glGetError();
    if (err != GL_NO_ERROR) {
        log->write("terrain-renderer", LogType::Warning, "GL error %#x", err);
    }
}
