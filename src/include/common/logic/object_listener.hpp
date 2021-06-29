#pragma once

/**
 * Object listener
 *
 * Since a lot of classes will need to listen for object actions, like
 * pathfinders (to see if an object moved), object pickers (to see the
 * object position) and others, this class will manage the object
 * listening
 *
 * (C) 2019 Arthur Mendes
 */

#include <set>

#include <common/logic/game_event.hpp>
#include <common/logic/game_object.hpp>

namespace familyline::logic
{
class ObjectListener : public EventReceiver
{
private:
    std::string _name = "object-listener";

    /// A set of id=>is_alive values
    ///
    /// I use a set so I don't need to worry about filtering
    /// duplicates
    std::set<object_id_t> _objects;

public:
    ObjectListener() : EventReceiver() {}

    virtual const std::string getName() { return _name; }

    /**
     * Update the object statuses according to the events
     */
    void updateObjects();

    void clear() { _objects.clear(); }
    
    std::set<object_id_t> getAliveObjects() const;

    ~ObjectListener();
};
}  // namespace familyline::logic
