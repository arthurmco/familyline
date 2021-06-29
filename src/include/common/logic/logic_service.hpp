#pragma once

#include <memory>

#include <common/logic/action_queue.hpp>
#include <common/logic/attack_manager.hpp>
#include <common/logic/debug_drawer.hpp>
#include <common/logic/object_factory.hpp>
#include <common/logic/object_listener.hpp>
#include <common/logic/object_manager.hpp>
#include <common/logic/terrain.hpp>

#include <common/logic/object_path_manager.hpp>

/**
 * Logic service class
 */
namespace familyline::logic
{
class LogicService
{
private:
    static std::unique_ptr<ActionQueue> _action_queue;
    static std::unique_ptr<AttackManager> _atk_manager;
    static std::unique_ptr<ObjectListener> _object_listener;
    static std::unique_ptr<DebugDrawer> _debug_drawer;
    static std::unique_ptr<ObjectFactory> _object_factory;

    static std::unique_ptr<ObjectPathManager> path_manager_;

    
public:
    static std::unique_ptr<ActionQueue>& getActionQueue();
    static std::unique_ptr<AttackManager>& getAttackManager();
    static std::unique_ptr<ObjectListener>& getObjectListener();

    static void initDebugDrawer(DebugDrawer*);
    static std::unique_ptr<DebugDrawer>& getDebugDrawer();

    static std::unique_ptr<ObjectFactory>& getObjectFactory();

    static void initPathManager(Terrain& t);
    static std::unique_ptr<ObjectPathManager>& getPathManager();
};

}  // namespace familyline::logic
