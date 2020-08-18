#pragma once

/**
 * Terrain render class
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <client/graphical/Texture.hpp>
#include <client/graphical/camera.hpp>
#include <common/logic/terrain.hpp>
#include <glm/glm.hpp>
#include <unordered_map>

namespace familyline::graphics
{
struct TerrainRenderInfo {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<unsigned int> indices;
    std::vector<unsigned int> texture_ids;
};

/**
 * Store information about the terrain textures we are going to use
 *
 * `tex` is the texture handle
 * `xscale` and `yscale` are the amount of "points" a single texture will
 * occupy, in the X or Y direction.
 */
struct TerrainTexInfo {
    Texture* tex;
    int xscale, yscale;
};

class TerrainRenderer
{
private:
    familyline::logic::Terrain& terr_;
    Camera& cam_;
    glm::mat4 _wmatrix = glm::mat4(1.0f);
    TerrainRenderInfo tri_;
    GLuint tvao_;

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

public:
    TerrainRenderer(familyline::logic::Terrain& terr, Camera& cam) : terr_(terr), cam_(cam) {}

    /**
     * Build the terrain textures
     */
    void buildTextures();

    void buildVertexData();

    /**
     * Render the terrain
     */
    void render();
};
}  // namespace familyline::graphics