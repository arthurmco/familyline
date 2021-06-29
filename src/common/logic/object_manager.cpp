#include <algorithm>
#include <common/logic/logic_service.hpp>
#include <common/logic/object_manager.hpp>

using namespace familyline::logic;

/**
 * Basic object event emitter
 */

ObjectEventEmitter::ObjectEventEmitter() : _name("object-event-emitter")
{
    LogicService::getActionQueue()->addEmitter((EventEmitter*)this);
}


/**
 * Notify the creation start
 *
 * The creation end (when we will send the Created event) will be sent when
 * the object gets rendered for the first time, or when the object gets fully
 * built
 */
void ObjectEventEmitter::notifyCreationStart(object_id_t id, const std::string& name)
{
    EntityEvent e{0, EventCreated{id}, this};
    this->pushEvent(e);
}

/**
 * Notify the removal
 *
 * This is when the object gets fully removed.
 * No more operations with it will be executed.
 */
void ObjectEventEmitter::notifyRemoval(object_id_t id, const std::string& name)
{
    (void)name;

    EntityEvent e{0, EventDestroyed{id}, this};
    this->pushEvent(e);
}

const std::string ObjectEventEmitter::getName() { return _name; }

ObjectManager::ObjectManager() { eventEmitter = new ObjectEventEmitter{}; }

ObjectManager::~ObjectManager()
{
    LogicService::getActionQueue()->removeEmitter(eventEmitter);
    delete eventEmitter;
}    


/**
 * Add an object to the manager.
 *
 * Adding means only getting the object an ID (therefore making
 * it valid) and adding it to the manager, so it can be updated
 * automatically at each game engine iteration
 *
 * Returns the ID
 */
object_id_t ObjectManager::add(std::shared_ptr<GameObject>&& o)
{
    auto nextID = ++_lastID;
    o->_id      = nextID;
    _objects.push_back(o);

    eventEmitter->notifyCreationStart(o->_id, o->getName());

    return nextID;
}

/**
 * Removes an object from the manager
 */
void ObjectManager::remove(object_id_t id)
{
    auto rit = std::remove_if(
        _objects.begin(), _objects.end(),
        [id](std::shared_ptr<GameObject>& v) { return v->getID() == id; });

    eventEmitter->notifyRemoval(id, (*rit)->getName());

    _objects.erase(rit);
}

/**
 * Update every object registered into the manager
 *
 * TODO: update in a certain order?
 */
void ObjectManager::update()
{
    for (auto& o : _objects) {
        o->update();
    }
}

std::optional<std::shared_ptr<GameObject>> ObjectManager::get(object_id_t id) const
{
    auto r = std::find_if(_objects.begin(), _objects.end(), [id](std::shared_ptr<GameObject> v) {
        return v->getID() == id;
    });

    if (r == _objects.end()) {
        return std::optional<std::shared_ptr<GameObject>>();
    }

    return std::make_optional(*r);
}
