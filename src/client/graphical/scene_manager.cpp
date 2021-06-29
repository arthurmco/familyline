#include <client/graphical/scene_manager.hpp>
#include <common/logger.hpp>
#include <memory>

using namespace familyline::graphics;

/**
 *  Update the vertices of the visible object into the
 * renderer and, consequently, on the video card.
 */
void SceneManager::updateObjectVertices(SceneObjectInfo& soi)
{
    auto vinfos  = soi.object->getVertexInfo();
    unsigned idx = 0;
    for (auto vhandle : soi.handles) {
        vhandle->vinfo = vinfos[idx];
        idx++;
    }

    if (soi.object->isVertexDataDirty()) {
        idx = 0;

        auto vgroups = soi.object->getVertexData();
        for (auto vhandle : soi.handles) {
            vhandle->update(vgroups[idx]);
            idx++;
        }
    }
}

void SceneManager::updateAnimations(SceneObjectInfo& soi, unsigned int ms)
{
    soi.object->stepAnimation(ms);
}

void SceneManager::update(unsigned int ms)
{
    for (auto& soi : objects_) {
        if (!soi.visible) continue;

        soi.object->update();
        this->updateAnimations(soi, ms);
        this->updateObjectVertices(soi);
    }
}

void SceneManager::remove(scene_object_handle_t meshHandle)
{
    auto& log = LoggerService::getLogger();
    auto iter = std::find_if(objects_.begin(), objects_.end(), [meshHandle](SceneObjectInfo& soi) {
        return soi.id == meshHandle;
    });

    if (iter == objects_.end()) return;

    for (auto h : iter->handles) {
        h->remove();
    }

    for (auto l : iter->lights) {
        renderer_.removeLight(l);
    }

    switch (iter->object->getType()) {
    case SceneObjectType::Light: {
        /// if it is a directional light, remove it and put the dark default light back
        auto slight = std::dynamic_pointer_cast<SceneObject<Light>>(iter->object);
            
        if (std::holds_alternative<SunLightType>(slight->getInner().getType())) {
            log->write(
                "scene-renderer", LogType::Info, "directional light removed");
            directionalLight_ = defaultDark;
        }
            
        break;
    }
    default: {
        
    }
    }
    
    log->write(
        "scene-renderer", LogType::Debug, "removed scene object %s with ID %#x",
        iter->object->getName().data(), meshHandle);

    objects_.erase(iter);
}

int SceneManager::add(std::shared_ptr<SceneObjectBase> so)
{
    auto& log = LoggerService::getLogger();
    int id    = (uintptr_t)so.get();

    auto vdata    = so->getVertexData();
    auto vinfo    = so->getVertexInfo();
    auto vhandles = std::vector<VertexHandle*>();
    auto vlights  = std::vector<LightHandle*>();;

    switch (so->getType()) {
        case SceneObjectType::Light: {
            /// if it is a directional light, save it
            auto slight = std::dynamic_pointer_cast<SceneObject<Light>>(so);
            
            if (std::holds_alternative<SunLightType>(slight->getInner().getType()) &&
                directionalLight_.getName().find("defaultdark") == 0) {
                
                if (slight->getInner().getName() == "defaultdark") {
                    log->write("scene-manager", LogType::Fatal, "This name is reserved.");
                    return -1;
                }
                
                directionalLight_ = slight->getInner();      
                log->write(
                    "scene-renderer", LogType::Info, "directional light identified");                
            }

            vlights.push_back(renderer_.createLight(slight->getInner()));            
            
            break;
        }
        case SceneObjectType::Mesh: {
            unsigned idx = 0;
            for (auto& vgroup : vdata) {
                vhandles.push_back(renderer_.createVertex(vgroup, vinfo[idx]));
                idx++;
            }
            break;
        }
        default:
            log->write(
                "scene-renderer", LogType::Error,
                "scene object '%s' (%#x) could not be added because it was invalid or unknown",
                so->getName().data(), id);
            return -1;
    }


    objects_.emplace_back(so, id, true, vhandles, vlights);

    log->write(
        "scene-renderer", LogType::Debug, "added scene object %s with ID %#x", so->getName().data(),
        id);

    return id;
}
