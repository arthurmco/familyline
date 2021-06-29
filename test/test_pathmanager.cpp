#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <common/logic/logic_service.hpp>
#include <common/logic/object_path_manager.hpp>
#include <common/logic/terrain.hpp>
#include <memory>

#include "utils.hpp"

using namespace familyline::logic;

class ObjectPathManagerTest : public ::testing::Test {
protected:
    TerrainFile tf;
    std::unique_ptr<Terrain> t;
    
    void SetUp() override {
        tf = TerrainFile{200, 200};
        t = std::make_unique<Terrain>(tf);

        LogicService::getActionQueue()->clearEvents();        
        LogicService::initDebugDrawer(new DummyDebugDrawer{*t});
        LogicService::initPathManager(*t);

    }

    // void TearDown() override {}

};

TEST_F(ObjectPathManagerTest, CanFindPath)
{

    ObjectManager om;

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         [&]() {},        atkComp};

    auto component = make_object(objParams);
    component->setPosition(glm::vec3(10, 1, 10));

    auto cid = om.add(std::move(component));

    glm::vec3 destination(30, 0, 30);

    auto& pm = LogicService::getPathManager();
    auto handle =
        pm->startPathing(*om.get(cid).value().get(), glm::vec2{destination.x, destination.z});
    EXPECT_EQ(PathStatus::NotStarted, pm->getPathStatus(handle));

    pm->update(om);
    EXPECT_NE(PathStatus::Invalid, pm->getPathStatus(handle));
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));

    for (int i = 0; i <= 20; i++) {
        EXPECT_NE(PathStatus::Repathing, pm->getPathStatus(handle)) << "Repathing in iteration " << i;
        pm->update(om);
    }

    EXPECT_NE(PathStatus::Invalid, pm->getPathStatus(handle));
    EXPECT_EQ(PathStatus::Completed, pm->getPathStatus(handle));

    {
        auto ncomp = om.get(cid).value();
        auto pos   = ncomp->getPosition();
        EXPECT_EQ(destination.x, pos.x);
        EXPECT_EQ(destination.y, pos.y);
        EXPECT_EQ(destination.z, pos.z);
    }
}

TEST_F(ObjectPathManagerTest, CanFindPathOnMultipleIterations)
{
    ObjectManager om;

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         [&]() {},        atkComp};

    auto component = make_object(objParams);
    component->setPosition(glm::vec3(10, 1, 10));

    auto cid = om.add(std::move(component));

    glm::vec3 destination(199, 0, 199);

    auto& pm = LogicService::getPathManager();
    pm->setItersPerFrame(50);
    auto handle =
        pm->startPathing(*om.get(cid).value().get(), glm::vec2{destination.x, destination.z});
    EXPECT_EQ(PathStatus::NotStarted, pm->getPathStatus(handle));

    pm->update(om);
    EXPECT_NE(PathStatus::Invalid, pm->getPathStatus(handle));
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));

    for (int i = 0; i <= 150; i++) {
        pm->update(om);
    }

    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));
    
    for (int i = 0; i <= 150; i++) {
        pm->update(om);
    }

    EXPECT_NE(PathStatus::Invalid, pm->getPathStatus(handle));
    EXPECT_EQ(PathStatus::Completed, pm->getPathStatus(handle));

    {
        auto ncomp = om.get(cid).value();
        auto pos   = ncomp->getPosition();
        EXPECT_EQ(destination.x, pos.x);
        EXPECT_EQ(destination.y, pos.y);
        EXPECT_EQ(destination.z, pos.z);
    }
}

TEST_F(ObjectPathManagerTest, IsPathFoundOnMultipleIterationsTheSameAsOneFoundIntoOne)
{
    ObjectManager om;

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         [&]() {},        atkComp};

    auto component = make_object(objParams);
    component->setPosition(glm::vec3(10, 1, 10));

    auto cid = om.add(std::move(component));

    glm::vec3 destination(199, 0, 199);

    auto& pm = LogicService::getPathManager();
    pm->setItersPerFrame(500);
    auto handle =
        pm->startPathing(*om.get(cid).value().get(), glm::vec2{destination.x, destination.z});
    EXPECT_EQ(PathStatus::NotStarted, pm->getPathStatus(handle));

    pm->update(om);
    EXPECT_NE(PathStatus::Invalid, pm->getPathStatus(handle));
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));

    for (int i = 0; i <= 150; i++) {
        pm->update(om);
    }

    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));
    
    for (int i = 0; i <= 150; i++) {
        pm->update(om);
    }

    EXPECT_NE(PathStatus::Invalid, pm->getPathStatus(handle));
    EXPECT_EQ(PathStatus::Completed, pm->getPathStatus(handle));

    {
        auto ncomp = om.get(cid).value();
        auto pos   = ncomp->getPosition();
        EXPECT_EQ(destination.x, pos.x);
        EXPECT_EQ(destination.y, pos.y);
        EXPECT_EQ(destination.z, pos.z);
    }
}

TEST_F(ObjectPathManagerTest, CanWalkAroundStaticEntities)
{
    ObjectManager om;

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         [&]() {},        atkComp};
    struct object_init obsParams = {"test-obj", "Obstacle", glm::vec2(3, 3), 100,
                                    100,        false,      [&]() {},        atkComp};

    auto component = make_object(objParams);
    auto obstacle  = make_object(obsParams);
    component->setPosition(glm::vec3(10, 1, 10));
    obstacle->setPosition(glm::vec3(20, 1, 20));

    auto cid = om.add(std::move(component));
    om.add(std::move(obstacle));

    glm::vec3 destination(30, 0, 30);

    auto& pm = LogicService::getPathManager();
    auto handle =
        pm->startPathing(*om.get(cid).value().get(), glm::vec2{destination.x, destination.z});

    LogicService::getActionQueue()->processEvents();
    pm->update(om);
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));

    for (int i = 0; i <= 35; i++) {
        LogicService::getActionQueue()->processEvents();
        pm->update(om);

        auto ncomp = om.get(cid).value();
        auto pos   = ncomp->getPosition();

        EXPECT_NE(std::tuple(20, 20), std::tie(pos.x, pos.z))
            << "X,Y position is inside the obstacle at iteration " << i;
        EXPECT_NE(std::tuple(19, 19), std::tie(pos.x, pos.z))
            << "X,Y position is inside the obstacle at iteration " << i;
        EXPECT_NE(std::tuple(21, 21), std::tie(pos.x, pos.z))
            << "X,Y position is inside the obstacle at iteration " << i;
    }

    EXPECT_EQ(PathStatus::Completed, pm->getPathStatus(handle));

    {
        auto ncomp = om.get(cid).value();
        auto pos   = ncomp->getPosition();
        EXPECT_EQ(destination.x, pos.x);
        EXPECT_EQ(destination.y, pos.y);
        EXPECT_EQ(destination.z, pos.z);
    }
}

TEST_F(ObjectPathManagerTest, CanWalkAroundMovingObstacles)
{
    ObjectManager om;

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         [&]() {},        atkComp};
    struct object_init obsParams = {"test-obj", "Obstacle", glm::vec2(3, 3), 100,
                                    100,        false,      [&]() {},        atkComp};

    auto component = make_object(objParams);
    auto obstacle  = make_object(obsParams);
    component->setPosition(glm::vec3(10, 1, 10));
    obstacle->setPosition(glm::vec3(10, 1, 20));

    auto cid = om.add(std::move(component));
    auto oid = om.add(std::move(obstacle));

    glm::vec3 destination(30, 0, 30);

    auto& pm = LogicService::getPathManager();
    auto handle =
        pm->startPathing(*om.get(cid).value().get(), glm::vec2{destination.x, destination.z});

    pm->startPathing(*om.get(oid).value().get(), glm::vec2{30, 20});

    LogicService::getActionQueue()->processEvents();
    pm->update(om);
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));

    for (int i = 0; i <= 45; i++) {
        LogicService::getActionQueue()->processEvents();
        pm->update(om);

        auto ncomp = om.get(cid).value();
        auto cpos  = ncomp->getPosition();
        auto nobs  = om.get(oid).value();
        auto opos  = nobs->getPosition();

        EXPECT_NE(std::tie(opos.x, opos.z), std::tie(cpos.x, cpos.z))
            << "X,Y position " << opos.x << ", " << opos.z << " is inside the moving obstacle at iteration " << i;
        EXPECT_NE(std::tuple(opos.x-1, opos.z-1), std::tie(cpos.x, cpos.z))
            << "X,Y (-1) position is inside the moving obstacle at iteration " << i;
        EXPECT_NE(std::tuple(opos.x+1, opos.z+1), std::tie(cpos.x, cpos.z))
            << "X,Y (+1) position is inside the moving obstacle at iteration " << i;
    }

    EXPECT_EQ(PathStatus::Completed, pm->getPathStatus(handle));

    {
        auto ncomp = om.get(cid).value();
        auto pos   = ncomp->getPosition();
        EXPECT_EQ(destination.x, pos.x);
        EXPECT_EQ(destination.y, pos.y);
        EXPECT_EQ(destination.z, pos.z);
    }
}

TEST_F(ObjectPathManagerTest, CanPathfindTwoObjectsSimultaneously)
{
    ObjectManager om;

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         [&]() {},        atkComp};
    struct object_init obsParams = {"test-obj", "Obstacle", glm::vec2(3, 3), 100,
                                    100,        false,      [&]() {},        atkComp};

    auto component1 = make_object(objParams);
    auto component2 = make_object(objParams);

    component1->setPosition(glm::vec3(30, 0, 60));
    component2->setPosition(glm::vec3(50, 0, 45));

    auto cid1 = om.add(std::move(component1));
    auto cid2 = om.add(std::move(component2));

    glm::vec3 destination1(10, 0, 30);
    glm::vec3 destination2(60, 0, 40);

    auto& pm = LogicService::getPathManager();
    auto handle1 =
        pm->startPathing(*om.get(cid1).value().get(), glm::vec2{destination1.x, destination1.z});
    auto handle2 =
        pm->startPathing(*om.get(cid2).value().get(), glm::vec2{destination2.x, destination2.z});

    LogicService::getActionQueue()->processEvents();
    pm->update(om);
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle1));
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle2));

    for (int i = 0; i <= 5; i++) {
        LogicService::getActionQueue()->processEvents();
        pm->update(om);
    }

    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle1));
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle2));

    for (int i = 0; i <= 15; i++) {
        LogicService::getActionQueue()->processEvents();
        pm->update(om);
    }

    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle1));
    EXPECT_EQ(PathStatus::Completed, pm->getPathStatus(handle2));

    for (int i = 0; i <= 60; i++) {
        LogicService::getActionQueue()->processEvents();
        pm->update(om);
    }

    EXPECT_EQ(PathStatus::Completed, pm->getPathStatus(handle1));
    EXPECT_EQ(PathStatus::Completed, pm->getPathStatus(handle2));

    {
        auto ncomp = om.get(cid1).value();
        auto pos   = ncomp->getPosition();
        EXPECT_EQ(destination1.x, pos.x);
        EXPECT_EQ(destination1.y, pos.y);
        EXPECT_EQ(destination1.z, pos.z);
    }
    {
        auto ncomp = om.get(cid2).value();
        auto pos   = ncomp->getPosition();
        EXPECT_EQ(destination2.x, pos.x);
        EXPECT_EQ(destination2.y, pos.y);
        EXPECT_EQ(destination2.z, pos.z);
    }
}

TEST_F(ObjectPathManagerTest, CanStopMovingOnImpossiblePath)
{
    ObjectManager om;

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         [&]() {},        atkComp};
    struct object_init obsParams = {"test-obj", "Test Obstacle", glm::vec2(2, 400), 100,
                                    100,        false,         [&]() {},        atkComp};

    auto component = make_object(objParams);
    auto obstacle = make_object(obsParams);

    glm::vec3 start(10, 1, 10);
    glm::vec3 destination(40, 0, 40);

    
    component->setPosition(start);
    obstacle->setPosition(glm::vec3(25, 1, 40));
    
    om.add(std::move(obstacle));
    auto cid = om.add(std::move(component));

    auto& pm = LogicService::getPathManager();
    auto handle =
        pm->startPathing(*om.get(cid).value().get(), glm::vec2{destination.x, destination.z});
    EXPECT_EQ(PathStatus::NotStarted, pm->getPathStatus(handle));

    LogicService::getActionQueue()->processEvents();
    pm->update(om);
    
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));
    
    for (int i = 0; i <= 5; i++) {
        LogicService::getActionQueue()->processEvents();
        pm->update(om);
    }

    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));

    for (int i = 0; i <= 80000 / pm->getItersPerFrame()  ; i++) {
        LogicService::getActionQueue()->processEvents();
        pm->update(om);
    }

    EXPECT_NE(PathStatus::Invalid, pm->getPathStatus(handle));
    EXPECT_EQ(PathStatus::Unreachable, pm->getPathStatus(handle));

    {
        auto ncomp = om.get(cid).value();
        auto pos   = ncomp->getPosition();
        EXPECT_NE(destination.x, pos.x);
        EXPECT_NE(destination.z, pos.z);
        EXPECT_NE(start.x, pos.x);
        EXPECT_NE(start.z, pos.z);
    }

}


TEST_F(ObjectPathManagerTest, CanPathEntityTwice)
{
    ObjectManager om;

    auto atkComp                 = std::optional<AttackComponent>();
    struct object_init objParams = {"test-obj", "Test Object", glm::vec2(3, 3), 100,
                                    100,        false,         [&]() {},        atkComp};


    glm::vec3 start(10, 1, 10);
    glm::vec3 destination(30, 0, 30);
    
    auto component = make_object(objParams);
    component->setPosition(start);

    auto cid = om.add(std::move(component));


    auto& pm = LogicService::getPathManager();
    auto handle =
        pm->startPathing(*om.get(cid).value().get(), glm::vec2{destination.x, destination.z});
    EXPECT_EQ(PathStatus::NotStarted, pm->getPathStatus(handle));

    pm->update(om);
    EXPECT_NE(PathStatus::Invalid, pm->getPathStatus(handle));
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));

    for (int i = 0; i <= 20; i++) {
        pm->update(om);
    }

    EXPECT_NE(PathStatus::Invalid, pm->getPathStatus(handle));
    EXPECT_EQ(PathStatus::Completed, pm->getPathStatus(handle));

    {
        auto ncomp = om.get(cid).value();
        auto pos   = ncomp->getPosition();
        EXPECT_EQ(destination.x, pos.x);
        EXPECT_EQ(destination.y, pos.y);
        EXPECT_EQ(destination.z, pos.z);
    }

    auto prevdest = destination;
    destination = glm::vec3(40, 0, 20);
    
    handle =
        pm->startPathing(*om.get(cid).value().get(), glm::vec2{destination.x, destination.z});
    EXPECT_EQ(PathStatus::Repathing, pm->getPathStatus(handle));

    pm->update(om);
    pm->update(om);
    pm->update(om);
    EXPECT_EQ(PathStatus::InProgress, pm->getPathStatus(handle));

    {
        auto ncomp = om.get(cid).value();
        auto pos   = ncomp->getPosition();
        EXPECT_EQ(prevdest.x+1, pos.x);
        EXPECT_EQ(prevdest.y, pos.y);
        EXPECT_EQ(prevdest.z-1, pos.z);
    }
    
    for (int i = 0; i <= 18; i++) {
        pm->update(om);
    }
        
    EXPECT_EQ(PathStatus::Completed, pm->getPathStatus(handle));

    {
        auto ncomp = om.get(cid).value();
        auto pos   = ncomp->getPosition();
        EXPECT_EQ(destination.x, pos.x);
        EXPECT_EQ(destination.y, pos.y);
        EXPECT_EQ(destination.z, pos.z);
    }
    
}
