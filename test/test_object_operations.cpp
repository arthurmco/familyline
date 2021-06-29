#include <gtest/gtest.h>

#include <common/logic/logic_service.hpp>
#include <common/logic/object_manager.hpp>
#include <variant>

#include "utils.hpp"

using namespace familyline::logic;

TEST(ObjectOps, ObjectAcquireID)
{
    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         []() {},         atkComp};

    auto component = make_object(objParams);

    ObjectManager om;
    auto id = om.add(std::move(component));

    ASSERT_GT(id, 0);

    auto retrievedComp = om.get(id);
    ASSERT_TRUE(retrievedComp.has_value());
    ASSERT_EQ(objParams.name, retrievedComp.value()->getName());
}

TEST(ObjectOps, ObjectRunsUpdate)
{
    bool updated = false;

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3),           100,
                                    100,        false,         [&]() {updated = true; }, atkComp};

    auto component = make_object(objParams);

    ObjectManager om;
    auto id = om.add(std::move(component));
    EXPECT_GT(id, 0);

    EXPECT_FALSE(updated);
    om.update();
    EXPECT_TRUE(updated);
}

TEST(ObjectOps, ObjectRemoves)
{
    bool updated = false;

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3),           100,
                                    100,        false,         [&]() {updated = true; }, atkComp};

    auto component = make_object(objParams);

    ObjectManager om;

    auto id = om.add(std::move(component));
    EXPECT_GT(id, 0);

    auto retrievedComp = om.get(id);
    EXPECT_TRUE(retrievedComp.has_value());

    om.remove(id);
    auto nullComp = om.get(id);
    EXPECT_FALSE(nullComp.has_value());
}

TEST(ObjectOps, ObjectTestIfCreationNotifies)
{
    auto& actionQueue = LogicService::getActionQueue();
    actionQueue->clearEvents();

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         [&]() {},        atkComp};

    ObjectManager om;
    auto component = make_object(objParams);
    auto id        = om.add(std::move(component));
    EXPECT_GT(id, 0);

    EntityEvent e;

    ObjectEventReceiver oer;
    EXPECT_FALSE(oer.pollEvent(e));

    actionQueue->addReceiver(
        &oer, {
                  ActionQueueEvent::Created,
              });

    actionQueue->processEvents();

    EXPECT_TRUE(oer.pollEvent(e));

    auto* ev = std::get_if<EventCreated>(&e.type);
    EXPECT_TRUE(ev);
    EXPECT_EQ(id, ev->objectID);

    actionQueue->removeReceiver(&oer);
}

TEST(ObjectOps, ObjectTestIfRemovalNotifies)
{
    auto& actionQueue = LogicService::getActionQueue();
    actionQueue->clearEvents();

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         [&]() {},        atkComp};

    ObjectManager om;
    auto component = make_object(objParams);
    auto id        = om.add(std::move(component));
    EXPECT_GT(id, 0);

    EntityEvent e;

    ObjectEventReceiver oer;
    EXPECT_FALSE(oer.pollEvent(e));

    actionQueue->addReceiver(
        &oer, {
                  ActionQueueEvent::Destroyed,
              });

    om.remove(id);

    actionQueue->processEvents();

    EXPECT_TRUE(oer.pollEvent(e));

    auto* ev = std::get_if<EventDestroyed>(&e.type);
    EXPECT_TRUE(ev);
    EXPECT_EQ(id, ev->objectID);

    actionQueue->removeReceiver(&oer);
}
