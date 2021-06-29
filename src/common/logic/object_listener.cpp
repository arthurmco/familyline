#include <common/logic/object_listener.hpp>

using namespace familyline::logic;

/**
 * Update the object statuses according to the events
 */
void ObjectListener::updateObjects()
{
    EntityEvent e = {};

    while (this->pollEvent(e)) {
        auto* evCreate = std::get_if<EventCreated>(&e.type);

        if (evCreate) {
            this->_objects.insert(evCreate->objectID);
            continue;
        }

        auto* evDestroy = std::get_if<EventDestroyed>(&e.type);

        if (evDestroy) {
            this->_objects.erase(evDestroy->objectID);
            continue;
        }
    }
}

std::set<object_id_t> ObjectListener::getAliveObjects() const { return _objects; }

#include <common/logic/logic_service.hpp>

ObjectListener::~ObjectListener() { LogicService::getActionQueue()->removeReceiver(this); }
