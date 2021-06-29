/**
 * Object lifecycle manager
 *
 * (C) 2019 Arthur Mendes
 */

#pragma once

#include <unordered_map>

#include "game_event.hpp"
#include "object_manager.hpp"

namespace familyline::logic
{
struct LifecycleData {
    std::weak_ptr<GameObject> obj;
    ActionQueueEvent event;

    int time_to_die = -1;
};

class Player;

/**
 * This class manages the game object lifecycle
 *
 * It is the main place where the events about the object state
 * are sent from. Other systems and classes need to use this
 * class if they wish to make things organized.
 *
 * This class also validates the events. For example, an object
 * can't be created, or destroyed, multiple times
 */
class ObjectLifecycleManager
{
private:
    ObjectManager &_om;

    std::unordered_map<object_id_t, LifecycleData> _o_creating;
    std::unordered_map<object_id_t, LifecycleData> _o_created;
    std::unordered_map<object_id_t, LifecycleData> _o_dying;
    std::unordered_map<object_id_t, LifecycleData> _o_dead;

public:
    ObjectLifecycleManager(ObjectManager &om) : _om(om) {}

    /**
     * Register the object. Return its ID
     *
     * Named doRegister because register is a reserved word
     */
    object_id_t doRegister(std::weak_ptr<GameObject>);

    /**
     * Notify the object creation
     *
     * This is different from registration, because this event
     * is usually sent when the object is fully built.
     *
     * For example, when you build a residence, you will call
     * register() when you tell the builder where to build said
     * residence, and call notifyCreation() when the residence
     * is fully built
     */
    void notifyCreation(object_id_t);

    /**
     * Notify that the object started dying
     *
     * You do not need to delete the object after you send this
     * event, because we need a time between the dying and dead
     * events, so that the animations can run
     *
     * After some ticks, the dead event will be sent and the
     * object will be deleted
     */
    void notifyDeath(object_id_t);

    /**
     * Update the things
     *
     * Run once per logic engine tick
     */
    void update();
};
}  // namespace familyline::logic
