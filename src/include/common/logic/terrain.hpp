#pragma once

#include <common/logic/terrain_file.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

/**
 * Terrain management class
 *
 * Copyright (C) 2020 Arthur Mendes
 */

namespace familyline::logic
{
/**
 * A nice way to bind additional data into the terrain, usually data
 * that has a value per slot, like pathfinding data, fog of war or, in
 * some other games, data like crime, pollution...
 *
 * The only difference is that the terrain owns this data instead of you,
 * so is one less thing to worry about.
 *
 * Also, we can color the overlay areas
 */
class TerrainOverlay
{
private:
    std::vector<uint16_t> data_;

public:
    TerrainOverlay(size_t size);

    decltype(data_)& getData() { return data_; }
};

///////////////////////////////////

/**
 * Terrain types.
 *
 * Under development
 */
enum TerrainType : uint16_t { Grass = 0, Sand = 10, Water = 20, Mountain = 30, Dirt = 40 };

class Terrain
{
private:
    TerrainFile& tf_;

    double xzscale_ = 0.50;
    double yscale_  = 0.01;

    std::unordered_map<std::string, std::unique_ptr<TerrainOverlay>> overlays_;

public:
    Terrain(TerrainFile& tf) : tf_(tf) {}

    /**
     * Convert game engine coordinates to opengl coordinates
     */
    glm::vec3 gameToGraphical(glm::vec3 gameCoords) const
    {
        return glm::vec3(gameCoords.x * xzscale_, gameCoords.y * yscale_, gameCoords.z * xzscale_);
    }

    /**
     * Get raw terrain data
     *
     * Note that you might not need to use this function.
     *
     * If what you want is just getting height data, use the
     * `getHeightFromCoords()` function
     */
    const std::vector<uint16_t>& getHeightData() const { return tf_.getHeightData(); }

    /**
     * Get raw terrain type data
     */
    const std::vector<uint16_t>& getTypeData() const { return tf_.getTypeData(); }

    /**
     * Get height coords from a set of X and Y coords in the map
     */
    unsigned getHeightFromCoords(glm::vec2 coords) const;

    /**
     * Convert opengl coordinates to game engine coordinates
     */
    glm::vec3 graphicalToGame(glm::vec3 gfxcoords) const
    {
        return glm::vec3(gfxcoords.x / xzscale_, gfxcoords.y / yscale_, gfxcoords.z / xzscale_);
    }

    std::tuple<uint32_t, uint32_t> getSize() const { return tf_.getSize(); }

    TerrainOverlay* createOverlay(const char* name);
};

}  // namespace familyline::logic
