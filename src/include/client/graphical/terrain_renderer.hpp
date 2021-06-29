#pragma once

/**
 * Terrain render class
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <client/graphical/camera.hpp>
#include <client/graphical/renderer.hpp>
#include <common/logic/terrain.hpp>

namespace familyline::graphics
{

class TerrainRenderer
{
protected:
    familyline::logic::Terrain* terr_;
    Camera& cam_;
    
public:
    TerrainRenderer(Camera& cam)
        : cam_(cam)
        {}

    /**
     * Sets the terrain
     *
     * We do this after the initialization so we can build this class from
     * the Renderer, and have a specific terrain renderer for each API
     * we want to support
     */
    void setTerrain(familyline::logic::Terrain* t) { terr_ = t; }

    /**
     * Build the terrain textures
     */
    virtual void buildTextures() = 0;

    virtual void buildVertexData() = 0;

    /**
     * Render the terrain
     *
     * We use the GL renderer to let it render the lights itself, and
     * so we do not duplicate code to render lights here
     */
    virtual void render(Renderer& rnd) = 0;

    virtual ~TerrainRenderer() {}
};
}  // namespace familyline::graphics
