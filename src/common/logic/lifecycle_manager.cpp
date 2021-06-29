#include <common/logger.hpp>
#include <common/logic/lifecycle_manager.hpp>

using namespace familyline::logic;

/// TODO: redo the whole way the ObjectState events are sent.
///
/// For example, the ObjectManager does not have worry about
/// sending events.


/**
 * Register the object. Return its ID
 *
 * Named doRegister because register is a reserved word
 */
object_id_t ObjectLifecycleManager::doRegister(std::weak_ptr<GameObject> o)
{
    LifecycleData lcd = {};
    lcd.obj           = o;
    lcd.event         = ActionQueueEvent::Created;

    assert(!o.expired());
    auto id         = o.lock()->getID();
    _o_creating[id] = lcd;

    return id;
}

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
void ObjectLifecycleManager::notifyCreation(object_id_t id)
{
    auto& log = LoggerService::getLogger();

    auto optr = _o_creating.find(id);
    if (optr == _o_creating.end()) {
        log->write(
            "lifecycle-manager", LogType::Warning,
            "Tried to notify creation of object id %ld, but it cannot be transferred to that state",
            id);
        return;
    }

    LifecycleData lcd = optr->second;
    lcd.event         = ActionQueueEvent::Created;

    _o_created[id] = lcd;
    _o_creating.erase(id);

    log->write("lifecycle-manager", LogType::Info, "Object with ID %ld has been created", id);
}

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
void ObjectLifecycleManager::notifyDeath(object_id_t id)
{
    auto& log = LoggerService::getLogger();

    auto optr = _o_created.find(id);
    if (optr == _o_created.end()) {
        log->write(
            "lifecycle-manager", LogType::Warning,
            "Tried to notify death of object id %ld, but it cannot be transferred to that state",
            id);
        return;
    }

    LifecycleData lcd = optr->second;
    lcd.event         = ActionQueueEvent::Dying;
    lcd.time_to_die   = 80;  // 80 ticks before removing the object

    _o_dying[id] = lcd;
    _o_created.erase(id);

    log->write("lifecycle-manager", LogType::Info, "object with ID %ld died", id);
}

/**
 * Update the things
 *
 * Run once per logic engine tick
 */
void ObjectLifecycleManager::update()
{
    auto& log = LoggerService::getLogger();

    // Remove the dead objects
    for (auto [id, lcd] : _o_dead) {
        _om.remove(id);
    }
    _o_dead.clear();

    // Update the dying countdown
    std::vector<object_id_t> dying_remove;
    for (auto& [id, lcd] : _o_dying) {
        lcd.time_to_die--;

        if (lcd.time_to_die <= 0) {
            lcd.event = ActionQueueEvent::Dead;

            log->write(
                "lifecycle-manager", LogType::Info,
                "object with ID %ld is dead and will be removed", id);

            _o_dead[id] = lcd;
            dying_remove.push_back(id);
        }
    }

    for (auto id : dying_remove) {
        _o_dying.erase(id);
    }
}
