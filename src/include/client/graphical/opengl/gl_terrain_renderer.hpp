#pragma once

/**
 * OpenGL specialization of the terrain render class
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <client/graphical/terrain_renderer.hpp>
#include <client/graphical/Texture.hpp>
#include <client/graphical/shader.hpp>
#include <common/logic/terrain.hpp>
#include <glm/glm.hpp>
#include <unordered_map>

namespace familyline::graphics
{
struct TerrainRenderInfo {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<unsigned int> texture_ids;

    std::vector<unsigned int> indices;
};

/**
 * Store information about the terrain textures we are going to use
 *
 * `tex` is the texture handle
 * `xscale` and `yscale` are the amount of "points" a single texture will
 * occupy, in the X or Y direction.
 */
struct TerrainTexInfo {
    std::string name;
    int xscale, yscale;
};

class GLTerrainRenderer : public TerrainRenderer
{
private:
    glm::mat4 _wmatrix = glm::mat4(1.0f);
    TerrainRenderInfo tri_;
    GLuint tvao_;

    Texture* tatlas_;

    /**
     * Create the indices.
     *
     * The indices make each of those squares go to the video card
     * in a clockwise order
     */
    std::vector<unsigned int> createIndices(
        const std::vector<glm::vec3>& vertices, int width) const;

    std::vector<glm::vec3> createNormals(const std::vector<glm::vec3>& vertices, int width) const;

    std::vector<TerrainTexInfo> terrain_data_;

    /**
     * Map a texture type to an texture data ID
     *
     * We do not map they directly, because we will introduce blended terrain
     * who have a mix of more than two types.
     */
    std::unordered_map<familyline::logic::TerrainType, unsigned int> terr_type_to_idx_;

    TerrainRenderInfo createTerrainData();

    /**
     * Create VAO from terrain data
     */
    GLuint createTerrainDataVAO();
    ShaderProgram* sTerrain_;

public:
    GLTerrainRenderer(Camera& cam);

    /**
     * Build the terrain textures
     */
    virtual void buildTextures();

    virtual void buildVertexData();

    /**
     * Render the terrain
     *
     * We use the GL renderer to let it render the lights itself, and
     * so we do not duplicate code to render lights here
     */
    virtual void render(Renderer& rnd);

    virtual ~GLTerrainRenderer() {}

};
}  // namespace familyline::graphics
