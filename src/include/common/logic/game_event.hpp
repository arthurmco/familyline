#pragma once

#include <memory>
#include <queue>
#include <string>
#include <variant>

namespace familyline::logic
{
// TODO: rename GameObject to GameEntity?

/*
 * These events are events that happen from the point of view of the
 * objects
 */
typedef unsigned long long entity_id_t;

/**
 * An object has been created.
 *
 * It does not means unit born, or construction built.
 * It only means the object has been added to the object manager
 */
struct EventCreated {
    entity_id_t objectID;
};

/**
 * An entity has been started to build
 *
 * This means a building has been started to be built.
 * This event also happens for units, but right before the "Built" event
 */
struct EventBuilding {
    entity_id_t objectID;
};

/**
 * An entity has been built
 *
 * This means a building finished build, or an unit has been born
 */
struct EventBuilt {
    entity_id_t objectID;
};

/**
 * The entity has been finished doing some action and it is waiting
 */
struct EventReady {
    entity_id_t objectID;
};

/**
 * The entity who sent the event is attacking another entity
 *
 * We also send the position of where the attacker and the defender was
 */
struct EventAttacking {
    entity_id_t attackerID;
    entity_id_t defenderID;

    unsigned int atkXPos, atkYPos;
    unsigned int defXPos, defYPos;

    double damageDealt;
};

/**
 * This entity is working, collecting resources
 */
struct EventWorking {
    entity_id_t objectID;

    unsigned int atkXPos, atkYPos;
};

/**
 * This entity is starting to get garrisoned or ungarrisoned
 *
 * The `parentID` is the entity that you will be entering/exiting, and
 * the `entering` value defines if you are entering to/exiting from the
 * entity.
 */
struct EventGarrisoned {
    entity_id_t objectID;

    entity_id_t parentID;
    bool entering;
};

/**
 * This entity just had its health points reduced to 0
 *
 * It started dying.
 * This event will be used to run the death animation
 */
struct EventDying {
    entity_id_t objectID;

    unsigned int atkXPos, atkYPos;
};

/**
 * This entity just ended the death animation. Only its remains lie at the
 * floor.
 *
 * Soon this entity will be cleaned from memory
 */
struct EventDead {
    entity_id_t objectID;
};

/**
 * The entity has been deallocated from memory
 *
 * You will not be able to use this ID. This is only for you to know
 * who has been deallocated, so you can remove this ID from *your*
 * bookkeeping lists
 */
struct EventDestroyed {
    entity_id_t objectID;
};

class EventEmitter;
class ActionQueue;

typedef std::variant<
    EventCreated, EventBuilding, EventBuilt, EventReady, EventAttacking, EventWorking,
    EventGarrisoned, EventDying, EventDead, EventDestroyed>
    EntityEventType;

struct EntityEvent {
    unsigned long long timestamp;
    EntityEventType type;
    EventEmitter* emitter;
};

/**
 * Event receiver
 */
class EventReceiver
{
protected:
    std::queue<EntityEvent> events;

public:
    virtual const std::string getName() = 0;

    /**
     * Push the event to the receiver internal queue, so it can be
     * pulled by anyone who is listening through this receiver
     */
    void pushEvent(EntityEvent& e);

    bool pollEvent(EntityEvent& e);

    virtual ~EventReceiver() {}
};

/**
 * Event emitter
 */
class EventEmitter
{
    friend class ActionQueue;

protected:
    ActionQueue* queue;

    /**
     * Pushes the event to the central action queue
     */
    void pushEvent(EntityEvent& e);

public:
    virtual const std::string getName() = 0;

    virtual ~EventEmitter() {}
};
}  // namespace familyline::logic
