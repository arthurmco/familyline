#include <client/graphical/opengl/gl_renderer.hpp>

#ifdef RENDERER_OPENGL

#include <client/graphical/exceptions.hpp>
#include <client/graphical/gfx_service.hpp>
#include <client/graphical/opengl/gl_terrain_renderer.hpp>
#include <client/graphical/shader_manager.hpp>
#include <common/logger.hpp>

using namespace familyline;
using namespace familyline::graphics;

GLRenderer::GLRenderer()
{
    auto& d = GFXService::getDevice();

    _sForward = d->createShaderProgram(
        "forward", {d->createShader("shaders/Forward.vert", ShaderType::Vertex),
                    d->createShader("shaders/Forward.frag", ShaderType::Fragment)});

    _sForward->link();

    _sLines = d->createShaderProgram(
        "lines", {d->createShader("shaders/Lines.vert", ShaderType::Vertex),
                  d->createShader("shaders/Lines.frag", ShaderType::Fragment)});

    _sLines->link();
}

VertexHandle* GLRenderer::createVertex(VertexData& vd, VertexInfo& vi)
{
    auto& log = LoggerService::getLogger();

    if (!vi.shaderState.shader)
        throw graphical_exception("Invalid shader detected while creating vertex");

    auto [vao, vboPos, vboNorm, vboTex] = this->createRaw(vd, *vi.shaderState.shader);
    auto vhandle                        = std::make_unique<GLVertexHandle>(vao, *this, vi);
    vhandle->vsize                      = vd.position.size();

    //    log->write(
    //        "gl-renderer", LogType::Debug, "created vertex handle: vao=%#x, vsize=%zu", vao,
    //        vhandle->vsize);

    vhandle->vboPos  = vboPos;
    vhandle->vboNorm = vboNorm;
    vhandle->vboTex  = vboTex;

    _vhandle_list.push_back(std::move(vhandle));

    return _vhandle_list.back().get();
}

void GLRenderer::render(Camera* c)
{
    this->removeScheduledVertices();
    
    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // we're not using the stencil buffer now

    auto& shaderManager = GFXService::getShaderManager();
    auto& log           = LoggerService::getLogger();

    this->runHooks(c);

    auto viewMatrix = c->GetViewMatrix();
    auto projMatrix = c->GetProjectionMatrix();

    for (auto& vh : _vhandle_list) {
        ShaderProgram* shader = vh->vinfo.shaderState.shader;
        shaderManager->use(*shader);

        if (directionalLight_) {
            Light& li = directionalLight_->light;
            shader->setUniform("dirColor", li.getColor());
            shader->setUniform("dirPower", li.getPower());
            shader->setUniform("dirDirection", std::get<SunLightType>(li.getType()).direction);
        }

        this->drawLights(*shader);

        shader->setUniform("mView", viewMatrix);
        shader->setUniform("mProjection", projMatrix);
        shader->setUniform("mvp", projMatrix * viewMatrix * glm::mat4(1.0));

        if (vh->vinfo.materialID >= 0) {
            Material* m     = GFXService::getMaterialManager()->getMaterial(vh->vinfo.materialID);
            MaterialData md = m->getData();

            shader->setUniform("diffuse_color", md.diffuseColor);
            shader->setUniform("ambient_color", md.ambientColor);
            shader->setUniform("diffuse_intensity", 1.0f);
            shader->setUniform("ambient_intensity", 1.0f);

            if (Texture* t = m->getTexture(); t) {
                glBindTexture(GL_TEXTURE_2D, t->GetHandle());
                shader->setUniform("tex_amount", 1.0f);

            } else {
                glBindTexture(GL_TEXTURE_2D, 0);
                shader->setUniform("tex_amount", 0.0f);
            }

            //            printf("<<<< %d dif=%.2f,%.2f,%.2f amb=%.2f,%.2f,%.2f  >>>>",
            //                   vh->vinfo.materialID,
            //                   md.diffuseColor.x, md.diffuseColor.y, md.diffuseColor.z,
            //                   md.ambientColor.x, md.ambientColor.y, md.ambientColor.z);

        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            shader->setUniform("tex_amount", 0.0f);
            shader->setUniform("diffuse_color", glm::vec3(0.5));
            shader->setUniform("ambient_color", glm::vec3(0.1));
            shader->setUniform("diffuse_intensity", 0.0f);
            shader->setUniform("ambient_intensity", 0.0f);
        }

        vh->vinfo.shaderState.updateShader();

        bool hasTexture = vh->vinfo.hasTexCoords;

        glBindVertexArray(vh->vao);

        auto fnGetAttrib = [&](const char* name) {
            glGetError();
            auto r = glGetAttribLocation(shader->getHandle(), name);

            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                log->write(
                    "gl-renderer", LogType::Error,
                    "OpenGL error %#x while searching for shader attribute %s (shader is invalid)",
                    err, name);
            }

            return r;
        };

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vh->vboPos);
        glVertexAttribPointer(
            fnGetAttrib("position"),
            3,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
        );

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, vh->vboNorm);
        glVertexAttribPointer(
            fnGetAttrib(shader->getName() == std::string_view{"lines"} ? "color" : "normal"),
            3,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
        );

        if (hasTexture && fnGetAttrib("texcoord") >= 0) {
            glEnableVertexAttribArray(2);
            glBindBuffer(GL_ARRAY_BUFFER, vh->vboTex);
            glVertexAttribPointer(
                fnGetAttrib("texcoord"),
                2,  // size
                GL_FLOAT, GL_FALSE, 0, (void*)0);
        }

        auto glFormat =
            vh->vinfo.renderStyle == VertexRenderStyle::Triangles ? GL_TRIANGLES : GL_LINE_STRIP;
        glDrawArrays(glFormat, 0, vh->vsize);
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            log->write("gl-renderer", LogType::Error, "OpenGL error %#x", err);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindVertexArray(0);

            return;
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}

/**
 * Set a shader to draw the available lights on
 */
void GLRenderer::drawLights(ShaderProgram& sp)
{
    sp.setUniform("dirColor", directionalLight_->light.getColor());
    sp.setUniform("dirPower", directionalLight_->light.getPower());
    sp.setUniform(
        "dirDirection", std::get<SunLightType>(directionalLight_->light.getType()).direction);

    int idx             = 0;
    const int maxLights = 4;

    char posstr[32];
    char colorstr[32];
    char strenstr[32];

    /// TODO: maybe order by the distance from the camera
    /// and render lights close to it first?
    for (auto& l : this->vlight_list_) {
        if (idx == maxLights) break;

        if (l.get() == this->directionalLight_) continue;

        sprintf(posstr, "lights[%d].position", idx);
        sprintf(colorstr, "lights[%d].color", idx);
        sprintf(strenstr, "lights[%d].strength", idx);

        auto type = l->light.getType();
        if (auto pl = std::get_if<PointLightType>(&type)) {
            sp.setUniform(posstr, pl->position);
        }

        sp.setUniform(colorstr, l->light.getColor());
        sp.setUniform(strenstr, l->light.getPower());
        idx++;
    }

    sp.setUniform("lightCount", idx);
}

/**
 * Create a raw VAO, with the vbox for position, normal and texture.
 *
 * Useful when we need to only retrieve the basic elements VAO, without an object
 */
std::tuple<int, int, int, int> GLRenderer::createRaw(VertexData& vd, ShaderProgram& shader)
{
    auto& log = LoggerService::getLogger();

    GLuint vao = -1;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vboPos = -1, vboNorm = -1, vboTex = -1;
    bool animated = true;

    auto fnGetAttrib = [&](const char* name) {
        glGetError();
        auto r = glGetAttribLocation(shader.getHandle(), name);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            log->write(
                "gl-renderer", LogType::Error,
                "OpenGL error %#x while searching for shader attribute %s on vertex set addition "
                "(shader is invalid)",
                err, name);
        }

        return r;
    };

    glGenBuffers(1, &vboPos);
    glBindBuffer(GL_ARRAY_BUFFER, vboPos);
    glBufferData(
        GL_ARRAY_BUFFER, vd.position.size() * sizeof(glm::vec3), vd.position.data(),
        (animated ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));
    glVertexAttribPointer(fnGetAttrib("position"), 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNorm);
    glBindBuffer(GL_ARRAY_BUFFER, vboNorm);
    glBufferData(
        GL_ARRAY_BUFFER, vd.normals.size() * sizeof(glm::vec3), vd.normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(
        fnGetAttrib(shader.getName() == std::string_view{"lines"} ? "color" : "normal"), 3,
        GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    if (vd.texcoords.size() > 0 && fnGetAttrib("texcoord") >= 0) {
        glGenBuffers(1, &vboTex);
        glBindBuffer(GL_ARRAY_BUFFER, vboTex);
        glBufferData(
            GL_ARRAY_BUFFER, vd.texcoords.size() * sizeof(glm::vec2), vd.texcoords.data(),
            GL_STATIC_DRAW);
        glVertexAttribPointer(fnGetAttrib("texcoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(2);
    }

    glBindVertexArray(0);

    //    log->write(
    //        "gl-renderer", LogType::Debug, "created vertex set: vao=%#x, vbos=%#x,%#x,%#x", vao,
    //        vboPos, vboNorm, vboTex);

    return std::tie(vao, vboPos, vboNorm, vboTex);
}

void GLRenderer::removeVertex(VertexHandle* vh)
{
    GLVertexHandle* gvh = dynamic_cast<GLVertexHandle*>(vh);
    if (gvh == nullptr) return;

    glDeleteVertexArrays(1, (GLuint*)&gvh->vao);
    glDeleteBuffers(1, (GLuint*)&gvh->vboPos);
    glDeleteBuffers(1, (GLuint*)&gvh->vboNorm);

    if (gvh->vboTex >= 0) glDeleteBuffers(1, (GLuint*)&gvh->vboTex);

    to_be_removed_handles_.push_back(gvh);
}

void GLRenderer::removeScheduledVertices()
{
    if (to_be_removed_handles_.empty())
        return;
    
    auto r = std::remove_if(
        _vhandle_list.begin(), _vhandle_list.end(),
        [this](const std::unique_ptr<GLVertexHandle>& handle) {
            auto it = std::find(this->to_be_removed_handles_.begin(),
                                this->to_be_removed_handles_.end(),
                                handle.get());
            
            return it != this->to_be_removed_handles_.end();
        });

    _vhandle_list.erase(r, _vhandle_list.end());

    to_be_removed_handles_.clear();
}


LightHandle* GLRenderer::createLight(Light& light)
{
    auto lhandle = std::make_unique<LightHandle>(light);
    auto ret     = lhandle.get();
    vlight_list_.push_back(std::move(lhandle));

    if (std::holds_alternative<SunLightType>(ret->light.getType())) directionalLight_ = ret;

    return ret;
}

void GLRenderer::removeLight(LightHandle* lh)
{
    if (directionalLight_ == lh) directionalLight_ = nullptr;

    auto r = std::remove_if(
        vlight_list_.begin(), vlight_list_.end(), [lh](const std::unique_ptr<LightHandle>& handle) {
            return (handle->light.getName() == lh->light.getName());
        });

    vlight_list_.erase(r, vlight_list_.end());
}

TerrainRenderer* GLRenderer::createTerrainRenderer(Camera& camera)
{
    return new GLTerrainRenderer{camera};
}

bool GLVertexHandle::update(VertexData& vd)
{
    glBindVertexArray(this->vao);
    glBindBuffer(GL_ARRAY_BUFFER, this->vboPos);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vd.position.size() * sizeof(glm::vec3), vd.position.data());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

bool GLVertexHandle::remove()
{
    this->_renderer.removeVertex((VertexHandle*)this);
    return true;
}

bool GLVertexHandle::recreate(VertexData& vd, VertexInfo& vi)
{
    auto [vao, vboPos, vboNorm, vboTex] = this->_renderer.createRaw(vd, *vi.shaderState.shader);
    auto err                            = glGetError();

    if (err == GL_NO_ERROR) {
        this->vao = vao;

        this->vboPos  = vboPos;
        this->vboNorm = vboNorm;
        this->vboTex  = vboTex;
        this->vinfo   = vi;

        return true;
    }

    return false;
}

#endif
