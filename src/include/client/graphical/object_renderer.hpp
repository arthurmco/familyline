#pragma once

/**
 * Renders objects by sending it to the graphics engine
 *
 * It is a "bridge" class between the logic and the graphical engine
 */

#include <common/logic/terrain.hpp>
#include <common/logic/game_object.hpp>
#include <common/logic/object_components.hpp>
#include <map>
#include <memory>
#include <vector>

#include <client/graphical/scene_manager.hpp>

namespace familyline::graphics
{
/**
 * This struct exists because C++ does not lets you create a map
 * with references, because references are, basically, constant pointers
 * (you can change the thing it points to, but you can't change it to
 * point to something else after you set its value)
 */
struct RendererSlot {
    familyline::logic::object_id_t id;
    std::weak_ptr<familyline::logic::GameObject> component;
    int meshHandle = 0;

    RendererSlot(familyline::logic::object_id_t id, std::weak_ptr<familyline::logic::GameObject> c)
        : id(id), component(c)
    {
    }
};

class ObjectRenderer
{
private:
    std::vector<RendererSlot> components;
    const familyline::logic::Terrain& _terrain;
    SceneManager& _sr;

public:
    ObjectRenderer(const familyline::logic::Terrain& t, SceneManager& sr) : _terrain(t), _sr(sr) {}

    void add(std::shared_ptr<familyline::logic::GameObject> o);
    void remove(familyline::logic::object_id_t id);

    /**
     * Check if we need to update the meshes
     */
    bool willUpdate() { return true; }

    void update();
};
}  // namespace familyline::graphics
