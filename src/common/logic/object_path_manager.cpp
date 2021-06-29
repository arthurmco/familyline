#include <algorithm>
#include <array>
#include <cassert>
#include <common/logger.hpp>
#include <common/logic/logic_service.hpp>
#include <common/logic/object_path_manager.hpp>
#include <iterator>

#include "common/logic/action_queue.hpp"
#include "common/logic/game_event.hpp"
#include "common/logic/types.hpp"

using namespace familyline::logic;

/*
 * TODO: maybe, instead of recalculating the path if we have multiple paths in the same map, we
 *       only recalculate if a collision would occur
 */

class PathEventReceiver : public EventReceiver
{
public:
    PathEventReceiver()
    {
        LogicService::getActionQueue()->addReceiver(
            this, {
                      ActionQueueEvent::Created,
                      ActionQueueEvent::Dead,
                      ActionQueueEvent::Destroyed,
                  });
    }

    PathEventReceiver(PathEventReceiver&) = delete;
    PathEventReceiver& operator=(PathEventReceiver&) = delete;

    ~PathEventReceiver() { LogicService::getActionQueue()->removeReceiver(this); }

    virtual const std::string getName() { return "path-event-receiver"; }
};

ObjectPathManager::ObjectPathManager(Terrain& t) : t_(t)
{
    auto [w, h]      = t_.getSize();
    obstacle_bitmap_ = std::vector<unsigned>(w * h, 0);
    obj_events_      = std::make_unique<PathEventReceiver>();
}

/**
 * Start pathing an object
 *
 * Returns a path handle
 *
 * This function also detects if you are pathing an object that is already been pathed.
 * If so, we replace the destination and return the handle for the old one
 */
PathHandle ObjectPathManager::startPathing(GameObject& o, glm::vec2 dest)
{
    // TODO: require some sort of MovementComponent to start pathing
    auto& l = LoggerService::getLogger();

    auto* existingref = findPathRefFromObject(o);
    if (existingref) {
        l->write(
            "object-path-manager", LogType::Info,
            "found an existing reference to '%s' (%d) in the pathing list", o.getName().c_str(),
            o.getID());
        existingref->status = PathStatus::Repathing;
        existingref->start  = existingref->position() ? *existingref->position()
                                                     : glm::vec2(
                                                           existingref->object->getPosition().x,
                                                           existingref->object->getPosition().z);
        existingref->end = dest;
        return existingref->handleval();
    } else {
        auto pathref = PathRef(o, t_, dest);
        l->write(
            "object-path-manager", LogType::Info,
            "adding reference to '%s' (%d) in the pathing list", o.getName().c_str(), o.getID());
        operations_.push_back(std::move(pathref));
        return pathref.handleval();
    }
}

/**
 * Find an existing path reference from an object
 *
 * Useful to see if we are repathing an object
 *
 * Returns the pointer to the ref if we found, nullptr if we did not found
 */
ObjectPathManager::PathRef* ObjectPathManager::findPathRefFromObject(object_id_t oid)
{
    auto v = std::find_if(operations_.begin(), operations_.end(), [&](PathRef& r) {
        return r.object->getID() == oid;
    });

    if (v == operations_.end()) return nullptr;

    return &(*v);
}

/**
 * Get the status of a pathing operation
 */
PathStatus ObjectPathManager::getPathStatus(PathHandle h)
{
    auto v = findPathRefFromHandle(h);
    if (v == operations_.end()) {
        return PathStatus::Invalid;
    }

    return v->status;
}

/**
 * Get the status of a pathing operation
 */
PathStatus ObjectPathManager::getPathStatus(const GameObject& o)
{
    auto* v = findPathRefFromObject(o);
    if (!v) return PathStatus::Invalid;

    return v->status;
}

/**
 * Find the path reference from a handle
 *
 * Returns an iterator pointing to the object, or the value of end() if the
 * handle was not found
 */
decltype(ObjectPathManager::operations_.begin()) ObjectPathManager::findPathRefFromHandle(
    PathHandle h)
{
    return std::find_if(
        operations_.begin(), operations_.end(), [&](PathRef& r) { return r.handleval() == h; });
}

/**
 * Remove the pathing operation
 *
 * Note that we will only remove the operation it its refcount is zero
 */
void ObjectPathManager::removePathing(PathHandle h)
{
    operations_.erase(
        std::remove_if(
            operations_.begin(), operations_.end(), [&](PathRef& r) { return r.handleval() == h; }),
        operations_.end());
}

/**
 * Update the pathing
 *
 * This means:
 *  - moving the entity to the next position in the path (the granularity is low, so
 *    do not worry).
 *      - In the future we will take into account some things like entity speed, etc.
 *  - removing paths that completed
 *  - detect if a path completed calculation in its timeslot. If it did not, we reschedule
 *    it to the next frame
 *  - update the terrain bitmaps (according to each path, in the future will be according to
 *    the FoV of each unit)
 */
void ObjectPathManager::update(const ObjectManager& om)
{
    auto& log = LoggerService::getLogger();
    updateObstacleBitmap(om);

    std::vector<PathHandle> toRemove;
    int movingEntities = 0;

    for (auto& op : operations_) {
        switch (op.status) {
            case PathStatus::NotStarted: {
                createPath(op);
                op.status = PathStatus::InProgress;
                break;
            }
            case PathStatus::InProgress: {
                auto isLastPosition = op.pathElements.size() == 1;
                auto currentPos     = updatePosition(op);
                movingEntities++;

                LogicService::getDebugDrawer()->drawPath(
                    op.pathElements.begin(), op.pathElements.end(), glm::vec4(0, 1, 0, 1));

                // It is InProgress but no more positions. This is not correct
                if (!currentPos) {
                    log->write(
                        "object-path-manager", LogType::Error,
                        "Status of pathing %d (object %d (%s)) is PathStatus::InProgress, but you "
                        "called it when no points are available",
                        op.handleval(), op.object->getID(), op.object->getName().c_str());
                    op.status = PathStatus::Invalid;
                } else if (isLastPosition) {
                    // TODO: make the pathfinder alert if the path was not reached, or was reached
                    // close enough
                    log->write(
                        "object-path-manager", LogType::Info, "Pathing of %d (%d - %s) completed!",
                        op.handleval(), op.object->getID(), op.object->getName().c_str());

                    if (!op.pathfinder->hasPossiblePath())
                        op.status = PathStatus::Unreachable;
                    else
                        op.status = PathStatus::Completed;

                } else {
                    if (op.pathfinder->maxIterReached()) {
                        op.status = PathStatus::Repathing;
                    }
                }

                break;
            }
            case PathStatus::Unreachable:
                log->write(
                    "object-path-manager", LogType::Warning,
                    "Path handle %d from (%.2f, %.2f) to (%.2f, %.2f) is not reachable",
                    op.handleval(), op.start.x, op.start.y, op.end.x, op.end.y);
                [[fallthrough]];
            case PathStatus::Stopped: [[fallthrough]];
            case PathStatus::Invalid: [[fallthrough]];  // there is not much we can do here...
            case PathStatus::Completed: {
                op.ticks_to_remove--;
                if (op.ticks_to_remove < 0) {
                    log->write(
                        "object-path-manager", LogType::Info,
                        "Scheduling removal of path handle %d from the list", op.handleval());
                    toRemove.push_back(op.handleval());
                }
                break;
            }
            case PathStatus::Repathing: {
                log->write(
                    "object-path-manager", LogType::Info,
                    "Pathing of %d (%d - %s) needs to be recalculated!", op.handleval(),
                    op.object->getID(), op.object->getName().c_str());

                if (!op.pathfinder->maxIterReached())
                    op.pathfinder->update(createBitmapForObject(*op.object, op.ratio), op.ratio);

                this->recalculatePath(op, true);
                op.status = PathStatus::InProgress;
                break;
            }
        }
    }

    // There is more than one entity moving, path recalculation is needed, so entities
    // do not collide with each other
    if (movingEntities > 1) {
        log->write(
            "object-path-manager", LogType::Info, "Recalculating path for %d entities",
            movingEntities);
        std::for_each(operations_.begin(), operations_.end(), [this](PathRef& r) {
            r.pathfinder->update(createBitmapForObject(*r.object, r.ratio), r.ratio);
            this->recalculatePath(r);
        });
    }

    if (toRemove.size() > 0) {
        log->write("object-path-manager", LogType::Info, "Removing %zu pathrefs", toRemove.size());

        operations_.erase(
            std::remove_if(
                operations_.begin(), operations_.end(),
                [&](PathRef& r) {
                    return std::find(toRemove.begin(), toRemove.end(), r.handleval()) !=
                           toRemove.end();
                }),
            operations_.end());
    }
}

/**
 * Create a path for an object whose path does not exist yet
 */
void ObjectPathManager::createPath(PathRef& r)
{
    r.pathfinder->update(createBitmapForObject(*r.object, r.ratio), r.ratio);
    auto elements =
        r.pathfinder->findPath(r.start, r.end, r.object->getSize(), max_iter_paths_per_frame_);
    assert(r.pathElements.size() == 0);
    r.pathElements.insert(r.pathElements.begin(), elements.begin(), elements.end());
}

/**
 * Create a path for an object whose path already exists, essentially redoing it
 */
void ObjectPathManager::recalculatePath(PathRef& r, bool force)
{
    auto& log = LoggerService::getLogger();
    log->write(
        "object-path-manager", LogType::Debug,
        "Recalculating path for handle %d (%s) (%zu remaining points)", r.handleval(),
        r.object->getName().c_str(), r.pathElements.size());

    if (r.pathElements.size() <= 2 && !force) return;

    auto pos2d = glm::vec2(r.object->getPosition().x, r.object->getPosition().z);
    if (r.pathElements.size() == 0) r.pathElements.push_back(pos2d);

    auto posv = r.position().value_or(pos2d);

    auto elements =
        r.pathfinder->findPath(posv, r.end, r.object->getSize(), max_iter_paths_per_frame_ / 2);
    r.pathElements.resize(elements.size());
    std::copy(elements.begin(), elements.end(), r.pathElements.begin());
}

/**
 * Update the position of an object
 */
std::optional<glm::vec2> ObjectPathManager::updatePosition(PathRef& r)
{
    auto pos = r.position();
    if (!pos) return std::nullopt;

    auto height = t_.getHeightFromCoords(*pos);

    setObjectOnBitmap(obstacle_bitmap_, *r.object, false);
    setObjectOnBitmap(obstacle_bitmap_, *pos, r.object->getSize(), true);

    LoggerService::getLogger()->write(
        "object-path-manager", LogType::Debug,
        "position of object id %d (%s) is now (%.2f, %d, %.2f)", r.object->getID(),
        r.object->getName().c_str(), pos->x, height, pos->y);

    assert(fabs(pos->x - r.object->getPosition().x) <= 1.5);
    assert(fabs(pos->y - r.object->getPosition().z) <= 1.5);

    r.object->setPosition(glm::vec3(pos->x, height, pos->y));
    mapped_objects_[r.object->getID()] = std::make_tuple<>(*pos, r.object->getSize());
    r.pathElements.pop_front();

    return pos;
}

/**
 * Update the global obstacle bitmap with the data from our event receiver
 */
void ObjectPathManager::updateObstacleBitmap(const ObjectManager& om)
{
    pollEntities(om);
    std::fill(obstacle_bitmap_.begin(), obstacle_bitmap_.end(), false);

    for (auto [id, data] : mapped_objects_) {
        auto pos  = std::get<0>(data);
        auto size = std::get<1>(data);

        setObjectOnBitmap(obstacle_bitmap_, pos, size, true);
    }
}

/**
 * Update the mapped object list, adding the recently added entities and removing
 * the destroyed entities.
 */
void ObjectPathManager::pollEntities(const ObjectManager& om)
{
    auto& log = LoggerService::getLogger();

    EntityEvent e;
    while (obj_events_->pollEvent(e)) {
        if (auto* ec = std::get_if<EventCreated>(&e.type); ec) {
            if (auto obj = om.get(ec->objectID); obj) {
                auto pos  = (*obj)->getPosition();
                auto size = (*obj)->getSize();

                log->write(
                    "object-path-manager", LogType::Debug,
                    "adding '%s' (%d) (pos %.1f, %.1f, %.1f)to the list of mapped objects (as an "
                    "obstacle)",
                    (*obj)->getName().c_str(), ec->objectID, (*obj)->getPosition().x,
                    (*obj)->getPosition().y, (*obj)->getPosition().z);

                mapped_objects_[ec->objectID] = std::make_tuple<>(glm::vec2(pos.x, pos.z), size);
            }
        }

        // We have this one only to set the pathref status, so it does not go away
        // immediately.
        if (auto* ec = std::get_if<EventDead>(&e.type); ec) {
            mapped_objects_.erase(ec->objectID);

            // if we have a reference to any removed object, destroy it!
            auto* ref = findPathRefFromObject(ec->objectID);
            if (ref) {
                log->write(
                    "object-path-manager", LogType::Info,
                    "pathing of handle %d (id %d) stopped, because the entity is dead",
                    ref->handleval(), ec->objectID);

                ref->status = PathStatus::Stopped;
            }

            if (auto* ec = std::get_if<EventDestroyed>(&e.type); ec) {
                mapped_objects_.erase(ec->objectID);

                log->write(
                    "object-path-manager", LogType::Debug,
                    "removing pathing handle of destroyed entity %d", ec->objectID);

                // if we have a reference to any removed object, destroy it!
                auto* ref = findPathRefFromObject(ec->objectID);
                if (ref) {
                    operations_.erase(
                        std::remove_if(
                            operations_.begin(), operations_.end(),
                            [&](PathRef& r) { return r.oid == ec->objectID; }),
                        operations_.end());
                }
            }
        }
    }
}

/**
 * Resize a object bitmap according to a certain ratio
 */

/**
 * Creates an obstacle bitmap for the current game object
 *
 * Currently, it only removes the actual object from the bitmap, but, in the future,
 * it will also consider the terrain type (e.g, insert water for units that only walk
 * on land)
 */
std::vector<bool> ObjectPathManager::createBitmapForObject(const GameObject& o, int ratio)
{
    /// TODO: if two objects are too close to each other, one might not be seen on the other's
    /// obstacle bitmap
    std::vector<unsigned> v{obstacle_bitmap_};
    setObjectOnBitmap(v, o, false, 1);

    std::vector<bool> nv(v.size() / (ratio*ratio), false);

    auto [w, h] = t_.getSize();
    for (auto y = 0; y < h; y++) {
        for (auto x = 0; x < w; x++) {
            nv[(y / ratio) * (w / ratio) + (x / ratio)] = v[y * w + x] > 0;
        }
    }

    return nv;
}

/**
 * Set the object data in the obstacle bitmap to a certain state
 */
void ObjectPathManager::setObjectOnBitmap(
    std::vector<bool>& bitmap, const GameObject& o, bool value, float ratio)
{
    auto pos2d = glm::vec2(o.getPosition().x, o.getPosition().z);
    setObjectOnBitmap(bitmap, pos2d, o.getSize(), value, ratio);
}
/**
 * Set the object data in the obstacle bitmap to a certain state
 */
void ObjectPathManager::setObjectOnBitmap(
    std::vector<bool>& bitmap, glm::vec2 pos, glm::vec2 size, bool value, float ratio)
{
    int posx = pos.x;
    int posy = pos.y;

    auto [width, height] = t_.getSize();

    for (int y = posy - double(size.y / 2); y < posy + double(size.y / 2); y++) {
        if (y < 0 || y >= height) continue;

        for (int x = posx - double(size.x / 2); x < posx + double(size.x / 2); x++) {
            if (x < 0 || x >= width) continue;

            bitmap[int(ceil(y / ratio)) * int(width / ratio) + int(ceil(x / ratio))] = value;
        }
    }
}

/**
 * Set the object data in the global obstacle bitmap to a certain state
 */
void ObjectPathManager::setObjectOnBitmap(
    std::vector<unsigned>& bitmap, const GameObject& o, bool value, float ratio)
{
    auto pos2d = glm::vec2(o.getPosition().x, o.getPosition().z);
    setObjectOnBitmap(bitmap, pos2d, o.getSize(), value, ratio);
}
/**
 * Set the object data in the global obstacle bitmap to a certain state
 */
void ObjectPathManager::setObjectOnBitmap(
    std::vector<unsigned>& bitmap, glm::vec2 pos, glm::vec2 size, bool value, float ratio)
{
    int posx = pos.x;
    int posy = pos.y;

    auto [width, height] = t_.getSize();

    for (int y = posy - double(size.y / 2); y < posy + double(size.y / 2); y++) {
        if (y < 0 || y >= height) continue;

        for (int x = posx - double(size.x / 2); x < posx + double(size.x / 2); x++) {
            if (x < 0 || x >= width) continue;

            auto& val = bitmap[int(ceil(y / ratio)) * int(width / ratio) + int(ceil(x / ratio))];

            if (value) {
                val++;
            } else {
                if (val > 0) val--;
            }
        }
    }
}
