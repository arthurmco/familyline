#include <algorithm>
#include <client/graphical/mesh.hpp>
#include <client/graphical/object_renderer.hpp>
#include <common/logger.hpp>
#include <common/logic/logic_service.hpp>

#include "client/graphical/scene_object.hpp"

using namespace familyline::logic;
using namespace familyline::graphics;

void ObjectRenderer::add(std::shared_ptr<GameObject> o)
{
    if (!o->getLocationComponent()) {
        return;
    }

    RendererSlot rs(o->getID(), o);
    auto mesh = dynamic_cast<Mesh*>(o->getLocationComponent().value().mesh.get());

    if (mesh) {
        rs.meshHandle = _sr.add(make_scene_object(*mesh));

        this->components.push_back(rs);
        printf("added mesh handle %x\n", rs.meshHandle);
    }
}

void ObjectRenderer::remove(object_id_t id)
{
    auto iter = std::find_if(
        components.begin(), components.end(), [id](RendererSlot rs) { return rs.id == id; });
    if (iter == components.end()) return;

    _sr.remove(iter->meshHandle);
    this->components.erase(iter);
}

#include <client/graphical/terrain_renderer.hpp>

void ObjectRenderer::update()
{
    std::vector<object_id_t> expired;

    for (auto& l : this->components) {
        if (l.component.expired()) {
            expired.push_back(l.id);
            continue;
        }

        auto comp = l.component.lock();
        if (comp->getLocationComponent()) {
            glm::vec3 pstart = comp->getPosition();
            glm::vec3 pend   = comp->getPosition();
            pstart.y += 5;
            pend.y += 5;

            // draw square for mesh bounding box
            std::shared_ptr<Mesh> gmesh =
                std::dynamic_pointer_cast<Mesh>(comp->getLocationComponent()->mesh);
            BoundingBox bb = gmesh->getBoundingBox();
            glm::vec4 vmin = glm::vec4(bb.minX, bb.minY, bb.minZ, 1);
            glm::vec4 vmax = glm::vec4(bb.maxX, bb.maxY, bb.maxZ, 1);

            glm::vec3 vmin3 = _terrain.graphicalToGame(glm::vec3(gmesh->getWorldMatrix() * vmin));
            glm::vec3 vmax3 = _terrain.graphicalToGame(glm::vec3(gmesh->getWorldMatrix() * vmax));

            vmin3.y = pstart.y;
            vmax3.y = pend.y;
            vmin3 += glm::vec3(-0.5, 0, -0.5);
            vmax3 += glm::vec3(0.5, 0, 0.5);

            LogicService::getDebugDrawer()->drawSquare(
                vmin3, vmax3, glm::vec4(0.8, 0, 0.5, 1), glm::vec4(0, 0, 0, 0));

            // draw square for object hitbox
            glm::vec3 halfsize = glm::vec3(comp->getSize().x / 2, 0, comp->getSize().y / 2);
            LogicService::getDebugDrawer()->drawSquare(
                pstart - halfsize, pend + halfsize, glm::vec4(0.1, 0, 1, 1), glm::vec4(0, 0, 0, 0));

            comp->getLocationComponent()->updateMesh(_terrain);
        }
    }

    for (auto id : expired) {
        this->remove(id);
    }
}
