#include <gtest/gtest.h>

#include <client/game.hpp>
#include <client/input/input_manager.hpp>
#include <common/logic/logic_service.hpp>
#include <filesystem>
#include <string>

#include "client/graphical/gfx_service.hpp"
#include "utils.hpp"
#include "utils/test_device.hpp"
#include "utils/test_framebuffer.hpp"
#include "utils/test_gui_manager.hpp"
#include "utils/test_renderer.hpp"
#include "utils/test_window.hpp"

using namespace familyline::logic;
using namespace familyline::graphics;
using namespace familyline::input;
using namespace familyline;

TEST(InputReproduceTest, TestIfInputReproduces)
{
    auto ipr = std::make_unique<InputProcessor>();
    InputService::setInputManager(std::make_unique<InputManager>(*ipr));

    LogicService::getObjectListener()->clear();
    LogicService::getActionQueue()->clearEvents();
    LogicService::getObjectFactory()->clear();
    GFXService::setDevice(std::make_unique<TestDevice>());

    std::string mapfile = TESTS_DIR "/terrain_test.flte";

    TestWindow* w = (TestWindow*) GFXService::getDevice()->createWindow(800, 600);
    w->createRenderer();
    GFXGameInit gi{
        w, GFXService::getDevice()->createFramebuffer("f3D", 800, 600),
        GFXService::getDevice()->createFramebuffer("fGUI", 800, 600),
        w->createGUIManager()};

    Game* g   = new Game(gi);
    auto& map = g->initMap(mapfile);

    auto atkc1 = std::optional<AttackComponent>(AttackComponent{
        nullptr, 1.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 2.0f, 1.0f, 3.14f});
    auto obj_s = make_ownable_object(
        {"testobj", "Test Object", glm::vec2(1, 1), 200, 200, true, []() {}, atkc1});

    auto& of = LogicService::getObjectFactory();
    of->addObject(obj_s.get());

    auto reprfile = TESTS_DIR "/reproduce_test.frec";
    auto irepr    = std::make_unique<InputReproducer>(reprfile);

    ASSERT_TRUE(irepr->open());
    ASSERT_TRUE(irepr->verifyObjectChecksums(of.get()));

    PlayerSession session = irepr->createPlayerSession(map);

    ASSERT_TRUE(session.players->get(1).has_value());
    ASSERT_STREQ("Test", (*session.players->get(1))->getName().data());

    {
        int cid   = 1;
        Colony& c = session.player_colony.at(cid);
        ASSERT_STREQ("Test", c.getName().data());
    }

    auto viewer = session.players->add(std::make_unique<DummyPlayer>(
        *session.players.get(), map, "Viewer", 0xffff,
        [&](size_t tick) -> std::vector<PlayerInputType> { return {}; }));

    {
        auto* player   = *(session.players->get(viewer));
        auto& alliance = session.colonies->createAlliance(std::string{player->getName()});
        auto& colony   = session.colonies->createColony(
            *player, 0xffffff, std::optional<std::reference_wrapper<Alliance>>{alliance});
        session.player_colony.emplace(viewer, std::reference_wrapper(colony));
    }

    g->initAssets();

    irepr->dispatchEvents((1000 / LOGIC_DELTA) * 1);
    g->initReproducer(std::move(irepr));
    
    g->initPlayers(
        std::move(session.players), std::move(session.colonies), session.player_colony, viewer);
    g->initObjectManager();
    g->initLoopData(viewer);

    for (auto i = 0; i < (1000 / LOGIC_DELTA) * 20; i++) g->runLoop();

    {
        auto obj = g->getObjectManager()->get(1);
        ASSERT_TRUE(obj.has_value());

        auto pos = (*obj)->getPosition();
        ASSERT_FLOAT_EQ(30, pos.x);
        ASSERT_FLOAT_EQ(30, pos.z);
        
        delete g;
        delete w;
    }

    LogicService::getObjectListener()->clear();
    LogicService::getActionQueue()->clearEvents();
    InputService::setInputManager(std::unique_ptr<InputManager>());
    GFXService::setDevice(std::unique_ptr<TestDevice>());
    LogicService::getObjectFactory()->clear();
}

TEST(InputReproduceTest, TestIfInputReproducerFailsOnBrokenFile)
{
    auto ipr = std::make_unique<InputProcessor>();
    InputService::setInputManager(std::make_unique<InputManager>(*ipr));

    LogicService::getObjectListener()->clear();
    LogicService::getActionQueue()->clearEvents();
    LogicService::getObjectFactory()->clear();
    GFXService::setDevice(std::make_unique<TestDevice>());

    std::string mapfile = TESTS_DIR "/terrain_test.flte";

    TestWindow* w = (TestWindow*) GFXService::getDevice()->createWindow(800, 600);
    w->createRenderer();
    GFXGameInit gi{
        w, GFXService::getDevice()->createFramebuffer("f3D", 800, 600),
        GFXService::getDevice()->createFramebuffer("fGUI", 800, 600),
        w->createGUIManager()};

    Game* g   = new Game(gi);
    auto& map = g->initMap(mapfile);

    auto atkc1 = std::optional<AttackComponent>(AttackComponent{
        nullptr, 1.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 2.0f, 1.0f, 3.14f});
    auto obj_s = make_ownable_object(
        {"testobj", "Test Object", glm::vec2(1, 1), 200, 200, true, []() {}, atkc1});

    auto& of = LogicService::getObjectFactory();
    of->addObject(obj_s.get());

    auto reprfile = TESTS_DIR "/broken_input_file.frec";
    auto irepr    = std::make_unique<InputReproducer>(reprfile);

    ASSERT_FALSE(irepr->open());

    delete g;
    delete w;

    LogicService::getObjectListener()->clear();
    LogicService::getActionQueue()->clearEvents();
    InputService::setInputManager(std::unique_ptr<InputManager>());
    GFXService::setDevice(std::unique_ptr<TestDevice>());
    LogicService::getObjectFactory()->clear();
}
