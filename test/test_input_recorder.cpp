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

TEST(InputRecorderTest, TestIfInputRecords)
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

    auto atkc1 = std::optional<AttackComponent>(AttackComponent{
        nullptr, 1.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 2.0f, 1.0f, 3.14f});
    auto obj_s = make_ownable_object(
        {"testobj", "Test Object", glm::vec2(0, 0), 200, 200, true, []() {}, atkc1});

    PlayerSession session = {};
    session.players       = std::make_unique<PlayerManager>();
    session.colonies      = std::make_unique<ColonyManager>();

    auto d = std::make_unique<DummyPlayer>(
        *session.players.get(), map, "Test", 1, [&](size_t tick) -> std::vector<PlayerInputType> {
            switch (tick) {
                case 2: return {logic::CreateEntity{"testobj", 20, 20}};
                case 4: return {logic::SelectAction{{1}}};
                case 6: return {logic::ObjectMove{30, 30}};
                case 8: return {logic::SelectAction{{}}};
                default: return {};
            }
        });
    auto i = session.players->add(std::move(d), false);
    ASSERT_NE(-1, i);

    auto* player   = *(session.players->get(i));
    auto& alliance = session.colonies->createAlliance("Dummy");
    auto& colony   = session.colonies->createColony(
        *player, 0xffffff, std::optional<std::reference_wrapper<Alliance>>{alliance});
    session.player_colony.emplace(i, std::reference_wrapper(colony));

    g->initAssets();

    auto& of = LogicService::getObjectFactory();
    of->addObject(obj_s.get());

    auto recordfile = TESTS_DIR "/artifacts/record_test.flte";
    if (std::filesystem::is_regular_file(recordfile)) {
        std::filesystem::remove(recordfile);
    }

    {
        auto ir = std::make_unique<InputRecorder>(*session.players.get());
        ASSERT_TRUE(ir->createFile(recordfile, of.get()));

        g->initRecorder(std::move(ir));

        g->initPlayers(
            std::move(session.players), std::move(session.colonies), session.player_colony, i);
        g->initObjectManager();
        g->initLoopData(i);

        for (auto i = 0; i < (1000 / LOGIC_DELTA) * 20; i++)
            g->runLoop();

        delete g;
        delete w;
    }

    {
        ASSERT_TRUE(std::filesystem::is_regular_file(recordfile));
        FILE* frec = fopen(recordfile, "rb");
        ASSERT_TRUE(frec != nullptr);

        char start[5] = {};
        uint32_t version = 0;
        fread(start, 4, 1, frec);
        ASSERT_STREQ(start, "FREC");

        fread(&version, 4, 1, frec);
        ASSERT_EQ(version, 1);


        char end[5] = {};
        uint32_t count = 0;
        uint32_t checksum = 0;

        fseek(frec, 0, SEEK_END);
        auto posend = ftell(frec);
        fseek(frec, posend-12, SEEK_SET);

        fread(end, 4, 1, frec);
        ASSERT_STREQ(end, "FEND");

        fread(&count, 4, 1, frec);
        ASSERT_EQ(count, 4);

        fread(&checksum, 4, 1, frec );
        ASSERT_NE(checksum, 0);

        fclose(frec);
    }
    
    LogicService::getActionQueue()->clearEvents();
    InputService::setInputManager(std::unique_ptr<InputManager>());
    GFXService::setDevice(std::unique_ptr<TestDevice>());
}
