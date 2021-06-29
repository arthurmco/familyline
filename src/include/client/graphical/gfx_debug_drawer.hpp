/***
 * Graphical implementation of the debug draw functions
 *
 * (C) 2019 Arthur Mendes
 */
#pragma once

#include <cstdint>
#include <unordered_map>

#include <client/graphical/renderer.hpp>
#include <common/logic/debug_drawer.hpp>

namespace familyline::graphics
{
struct VHData {
    VertexHandle *handle;
    uint64_t last_tick;
};

class GFXDebugDrawer : public familyline::logic::DebugDrawer
{
private:
    uint64_t last_tick = 0;
    Renderer &_renderer;
    std::unordered_map<uint64_t, VHData> _vhandles;

public:
    GFXDebugDrawer(Renderer &r, const familyline::logic::Terrain &terr)
        : _renderer(r), DebugDrawer(terr)
    {}

    virtual void drawLine(glm::vec3 start, glm::vec3 end, glm::vec4 color);
    virtual void drawSquare(
        glm::vec3 start, glm::vec3 end, glm::vec4 foreground, glm::vec4 background);
    virtual void drawCircle(
        glm::vec3 point, glm::vec3 radius, glm::vec4 foreground, glm::vec4 background);

    /// Update some internal structure
    virtual void update();

    virtual ~GFXDebugDrawer() {}

};
}  // namespace familyline::graphics
