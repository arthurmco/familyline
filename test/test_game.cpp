#include <gtest/gtest.h>

#include <client/game.hpp>
#include <client/input/input_manager.hpp>
#include <common/logic/logic_service.hpp>
#include <string>

#include "client/graphical/gfx_service.hpp"
#include "utils.hpp"
#include "utils/test_framebuffer.hpp"
#include "utils/test_gui_manager.hpp"
#include "utils/test_renderer.hpp"
#include "utils/test_device.hpp"
#include "utils/test_window.hpp"

using namespace familyline::logic;
using namespace familyline::graphics;
using namespace familyline::input;
using namespace familyline;

TEST(GameTest, TestIfGameStartsAndRuns)
{    
    InputProcessor* ipr = new InputProcessor;
    InputService::setInputManager(std::make_unique<InputManager>(*ipr));
        
    LogicService::getActionQueue()->clearEvents();
    LogicService::getObjectFactory()->clear();
    GFXService::setDevice(std::make_unique<TestDevice>());
    
    std::string mapfile = TESTS_DIR "/terrain_test.flte";

    TestWindow* w = new TestWindow{};
    w->createRenderer();
    GFXGameInit gi{
        w, new TestFramebuffer{"f3D", 800, 600}, new TestFramebuffer{"fGUI", 800, 600},
        w->createGUIManager()};

    Game* g   = new Game(gi);
    auto& map = g->initMap(mapfile);

    PlayerSession session = {};
    session.players       = std::make_unique<PlayerManager>();
    session.colonies      = std::make_unique<ColonyManager>();

    auto d = std::make_unique<DummyPlayer>(
        *session.players.get(), map, "Test", 1, [&](size_t) -> std::vector<PlayerInputType> {
            return {};
        });
    auto i         = session.players->add(std::move(d));
    ASSERT_NE(-1, i);
    
    auto* player   = *(session.players->get(i));
    auto& alliance = session.colonies->createAlliance("Dummy");
    auto& colony   = session.colonies->createColony(
        *player, 0xffffff, std::optional<std::reference_wrapper<Alliance>>{alliance});
    session.player_colony.emplace(i, std::reference_wrapper(colony));

    g->initAssets();
    g->initObjectFactory();
    g->initPlayers(std::move(session.players), std::move(session.colonies),
                   session.player_colony, i);
    g->initObjectManager();
    g->initLoopData(i);
    
    for (auto i = 0; i < 120; i++) {
        g->runLoop();
    }

    ASSERT_NE(-1, i);

    delete g;
    delete w;

    LogicService::getActionQueue()->clearEvents();
    InputService::setInputManager(std::unique_ptr<InputManager>());
    GFXService::setDevice(std::unique_ptr<TestDevice>());
}
