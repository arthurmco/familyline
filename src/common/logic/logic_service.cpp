#include <common/logic/logic_service.hpp>

using namespace familyline::logic;

std::unique_ptr<ActionQueue> LogicService::_action_queue;
std::unique_ptr<AttackManager> LogicService::_atk_manager;
std::unique_ptr<ObjectListener> LogicService::_object_listener;
std::unique_ptr<DebugDrawer> LogicService::_debug_drawer;
std::unique_ptr<ObjectFactory> LogicService::_object_factory;

std::unique_ptr<ActionQueue>& LogicService::getActionQueue()
{
    if (!_action_queue) {
        _action_queue = std::make_unique<ActionQueue>();
    }

    return _action_queue;
}

std::unique_ptr<AttackManager>& LogicService::getAttackManager()
{
    if (!_atk_manager) {
        _atk_manager = std::make_unique<AttackManager>();
    }

    return _atk_manager;
}

std::unique_ptr<ObjectListener>& LogicService::getObjectListener()
{
    if (!_object_listener) {
        _object_listener = std::make_unique<ObjectListener>();
        getActionQueue()->addReceiver(
            _object_listener.get(), {
                ActionQueueEvent::Created,
                ActionQueueEvent::Destroyed,
            });
    }

    return _object_listener;
}

void LogicService::initDebugDrawer(DebugDrawer* d)
{
    _debug_drawer = std::unique_ptr<DebugDrawer>(d);
}

std::unique_ptr<DebugDrawer>& LogicService::getDebugDrawer() { return _debug_drawer; }

std::unique_ptr<ObjectFactory>& LogicService::getObjectFactory()
{
    if (!_object_factory) {
        _object_factory = std::make_unique<ObjectFactory>();
    }

    return _object_factory;
}

std::unique_ptr<ObjectPathManager> LogicService::path_manager_;

void LogicService::initPathManager(Terrain& t)
{
    if (path_manager_)
        path_manager_.reset();
        
    path_manager_ = std::make_unique<ObjectPathManager>(t);
    
}
std::unique_ptr<ObjectPathManager>& LogicService::getPathManager()
{
    return path_manager_;
}
