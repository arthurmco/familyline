/**
 * Object path manager
 * Acts as a higher level interface to the pathfinder
 *
 * Copyright (C) 2021 Arthur Mendes
 */
#pragma once

#include <common/logic/game_event.hpp>
#include <common/logic/game_object.hpp>
#include <common/logic/pathfinder.hpp>
#include <common/logic/terrain.hpp>
#include <common/logic/types.hpp>
#include <deque>
#include <glm/fwd.hpp>
#include <optional>
#include <vector>

#include "common/logic/object_manager.hpp"

namespace familyline::logic
{
using PathHandle = unsigned long int;

enum class PathStatus {

    /// Pathing operation completed
    Completed,

    /// Pathing operation not started
    NotStarted,

    /// The entity is still following the path
    InProgress,

    /// The pathing was redone by the user, it clicked to move an object and
    /// immediately clicked again somewhere
    Repathing,

    /// The pathing operation detected that the current path cannot be created.
    /// It stopped in the best position it detected.
    Unreachable,

    /// The pathing operation has been stopped, because the pathing object
    /// is dead
    Stopped,

    /// Pathing operation is invalid
    Invalid
};

// TODO: handle path appending (aka defining path waypoints, like in other RTS games)

/**
 * Does management of pathing, by creating paths and moving the objects along it
 */
class ObjectPathManager
{
public:
    ObjectPathManager(Terrain& t);

    ObjectPathManager(const ObjectPathManager&) = delete;
    ObjectPathManager& operator=(const ObjectPathManager&) = delete;

    struct PathRef {
        std::unique_ptr<Pathfinder> pathfinder;

        /// A pointer to a game object
        /// Apparently, we cannot use a reference here, because moving a reference deletes the move
        /// constructor
        GameObject* object;
        object_id_t oid;
        PathStatus status;
        glm::vec2 original_start;
        glm::vec2 start;
        glm::vec2 end;

        int ticks_to_remove = 300;

        /// Current terrain ratio
        int ratio = 2;        
        
        /// A path element deque (because pure queues does not allow copy to it)
        /// The front element is always where the object is.
        std::deque<glm::vec2> pathElements;
       
        /// The pathing calculation is completed. We now just follow the path
        bool calculationCompleted = false;

        int refcount = 2;

        bool updated = false;
        bool reinserted = false;

        /// Minimum amount of ticks to pass until we can repath again
        int minimum_next_repath = 0;

        PathRef(GameObject& o, Terrain& t, glm::vec2 dest)
            : object(&o),
              oid(o.getID()),
              pathfinder(std::make_unique<Pathfinder>(t)),
              original_start(glm::vec2(o.getPosition().x, o.getPosition().z)),
              start(glm::vec2(o.getPosition().x, o.getPosition().z)),
              status(PathStatus::NotStarted),
              ratio(2),
              end(dest)
        {
        }

        PathRef(const PathRef& other) = delete;
        PathRef& operator=(const PathRef& other) = delete;
        PathRef(PathRef& other)                  = delete;
        PathRef& operator=(PathRef& other) = delete;

        PathRef(PathRef&& other) = default;
        PathRef& operator=(PathRef&& other) = default;

        std::optional<glm::vec2> position() const
        {
            if (pathElements.empty()) return std::nullopt;

            return std::make_optional<>(pathElements.front());
        }

        PathHandle handleval() const { return (PathHandle)(oid * 2); }
    };

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
    void update(const ObjectManager& om);

    /**
     * Start pathing an object
     *
     * Returns a path handle
     *
     * This function also detects if you are pathing an object that is already been pathed.
     * If so, we replace the destination and return the handle for the old one
     */
    PathHandle startPathing(GameObject& o, glm::vec2 dest);

    /**
     * Get the status of a pathing operation
     */
    PathStatus getPathStatus(PathHandle);

    /**
     * Get the status of a pathing operation (overload for an object. Since only one pathing
     * operation per object is allowed, this operation is not ambiguous)
     */
    PathStatus getPathStatus(const GameObject& o);

    /**
     * Remove the pathing operation
     *
     * Note that we will only remove the operation it its refcount is zero
     */
    void removePathing(PathHandle);

    void setItersPerFrame(int v) { max_iter_paths_per_frame_ = v; }
    int getItersPerFrame() const { return max_iter_paths_per_frame_; }

private:
    Terrain& t_;
    std::unique_ptr<EventReceiver> obj_events_;

    int max_iter_paths_per_frame_ = 200;

    /**
     * A map of object IDs and their respective positions and sizes, to mask them into the
     * obstacle bitmap
     *
     * TODO: add an event to the action queue for position changed?
     */
    std::unordered_map<object_id_t, std::tuple<glm::vec2 /* pos */, glm::vec2 /* size */>>
        mapped_objects_;

    /**
     * Each element for this global obstacle 'bitmap' is a value telling how many entities
     * would be there.
     *
     * Useful for correctly hiding the obstacle bitmap in certain situations where two meshes would
     * be over each other
     */
    std::vector<unsigned> obstacle_bitmap_;

    /**
     * Find an existing path reference from an object
     *
     * Useful to see if we are repathing an object
     *
     * Returns the pointer to the ref if we found, nullptr if we did not found
     */
    PathRef* findPathRefFromObject(const GameObject& o) { return findPathRefFromObject(o.getID()); }

    /**
     * Find an existing path reference from an object
     *
     * Useful to see if we are repathing an object
     *
     * Returns the pointer to the ref if we found, nullptr if we did not found
     */
    PathRef* findPathRefFromObject(object_id_t oid);

    std::vector<ObjectPathManager::PathRef> operations_;

    /**
     * Find the path reference from a handle
     *
     * Returns an iterator pointing to the object, or the value of end() if the
     * handle was not found
     */
    decltype(operations_.begin()) findPathRefFromHandle(PathHandle);

    /**
     * Create a path for an object whose path does not exist yet
     */
    void createPath(PathRef& r);

    /**
     * Create a path for an object whose path already exists, essentially redoing it
     */
    void recalculatePath(PathRef& r, bool force = false);

    /**
     * Update the position of an object
     *
     * Returns the current position of said object, or nullopt if the object
     * had no more positions to go when the function was called
     */
    std::optional<glm::vec2> updatePosition(PathRef& r);

    /**
     * Update the mapped object list, adding the recently added entities and removing
     * the destroyed entities.
     */
    void pollEntities(const ObjectManager& om);

    /**
     * Update the global obstacle bitmap with the data from our event receiver
     */
    void updateObstacleBitmap(const ObjectManager& om);

    /**
     * Creates an obstacle bitmap for the current game object
     *
     * Currently, it only removes the actual object from the bitmap, but, in the future,
     * it will also consider the terrain type (e.g, insert water for units that only walk
     * on land)
     */
    std::vector<bool> createBitmapForObject(const GameObject& o, int ratio=1);

    /**
     * Set the object data in the obstacle bitmap to a certain state
     */
    void setObjectOnBitmap(std::vector<bool>& bitmap, const GameObject& o, bool value, float ratio=1.0);

    /**
     * Set the object data in the obstacle bitmap to a certain state
     */
    void setObjectOnBitmap(std::vector<bool>& bitmap, glm::vec2 pos, glm::vec2 size, bool value, float ratio=1.0);
    
    /**
     * Set the object data in the global obstacle bitmap to a certain state
     */
    void setObjectOnBitmap(std::vector<unsigned>& bitmap, const GameObject& o, bool value, float ratio=1.0);

    /**
     * Set the object data in the global obstacle bitmap to a certain state
     */
    void setObjectOnBitmap(std::vector<unsigned>& bitmap, glm::vec2 pos, glm::vec2 size, bool value, float ratio=1.0);
};

}  // namespace familyline::logic
