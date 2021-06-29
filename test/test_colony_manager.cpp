#include <gtest/gtest.h>

#include <common/logic/colony_manager.hpp>
#include <common/logic/lifecycle_manager.hpp>
#include <common/logic/logic_service.hpp>
#include <common/logic/player.hpp>
#include <common/logic/player_manager.hpp>

#include "utils.hpp"

using namespace familyline::logic;

TEST(ColonyManager, TestIfColonyAdds)
{
    ColonyManager cm;
    PlayerManager pm;

    TerrainFile tf{1,1};
    Terrain t{tf};

    auto p1 = std::make_unique<DummyPlayer>(
        pm, t, "Test1", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p2 = std::make_unique<DummyPlayer>(
        pm, t, "Test2", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p3 = std::make_unique<DummyPlayer>(
        pm, t, "Test3", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });

    Colony& c1 = cm.createColony(*p1.get(), 0xffff00ff, std::nullopt);
    Colony& c2 = cm.createColony(*p2.get(), 0xff0000ff, std::nullopt);
    Colony& c3 = cm.createColony(*p3.get(), 0xff00ffff, std::nullopt);

    ASSERT_EQ(c1.getName(), p1->getName());
    ASSERT_EQ(c2.getName(), p2->getName());
    ASSERT_EQ(c3.getName(), p3->getName());
}

TEST(ColonyManager, TestIfAllianceAdds)
{
    ColonyManager cm;
    PlayerManager pm;

    TerrainFile tf{1,1};
    Terrain t{tf};

    auto p1 = std::make_unique<DummyPlayer>(
        pm, t, "Test1", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p2 = std::make_unique<DummyPlayer>(
        pm, t, "Test2", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p3 = std::make_unique<DummyPlayer>(
        pm, t, "Test3", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });

    Alliance& a1 = cm.createAlliance("Alliance1");
    Alliance& a2 = cm.createAlliance("ANewAlliance2");

    Colony& c1 =
        cm.createColony(*p1.get(), 0xffff00ff, std::optional<std::reference_wrapper<Alliance>>{a1});
    Colony& c2 =
        cm.createColony(*p2.get(), 0xff0000ff, std::optional<std::reference_wrapper<Alliance>>{a2});
    Colony& c3 =
        cm.createColony(*p3.get(), 0xff00ffff, std::optional<std::reference_wrapper<Alliance>>{a1});

    ASSERT_EQ(c1.getAlliance().getID(), a1.getID());
    ASSERT_EQ(c2.getAlliance().getID(), a2.getID());
    ASSERT_EQ(c3.getAlliance().getID(), a1.getID());
    ASSERT_NE(c1.getAlliance().getID(), c2.getAlliance().getID());
    ASSERT_EQ(c1.getAlliance().getID(), c3.getAlliance().getID());
    ASSERT_NE(a1.getID(), a2.getID());
}

TEST(ColonyManager, TestIfAllianceChangesOneSideAlly)
{
    ColonyManager cm;
    PlayerManager pm;

    TerrainFile tf;
    tf.open(TESTS_DIR "/minimal_terrain.flte");
    Terrain t{tf};

    auto p1 = std::make_unique<DummyPlayer>(
        pm, t, "Test1", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p2 = std::make_unique<DummyPlayer>(
        pm, t, "Test2", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p3 = std::make_unique<DummyPlayer>(
        pm, t, "Test3", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });

    Alliance& a1 = cm.createAlliance("Alliance1");
    Alliance& a2 = cm.createAlliance("Alliance2");

    Colony& c1 =
        cm.createColony(*p1.get(), 0xffff00ff, std::optional<std::reference_wrapper<Alliance>>{a1});
    Colony& c2 =
        cm.createColony(*p2.get(), 0xff0000ff, std::optional<std::reference_wrapper<Alliance>>{a2});
    Colony& c3 =
        cm.createColony(*p3.get(), 0xff00ffff, std::optional<std::reference_wrapper<Alliance>>{a1});

    cm.setAllianceDiplomacy(c1.getAlliance(), c2.getAlliance(), DiplomacyStatus::Ally);
    ASSERT_EQ(DiplomacyStatus::Ally, cm.getDiplomacy(c1, c2));
    ASSERT_EQ(DiplomacyStatus::Neutral, cm.getDiplomacy(c2, c1));
}

TEST(ColonyManager, TestIfAllianceChangesOneSideEnemy)
{
    ColonyManager cm;
    PlayerManager pm;

    TerrainFile tf{1,1};
    Terrain t{tf};

    auto p1 = std::make_unique<DummyPlayer>(
        pm, t, "Test1", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p2 = std::make_unique<DummyPlayer>(
        pm, t, "Test2", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p3 = std::make_unique<DummyPlayer>(
        pm, t, "Test3", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });

    Alliance& a1 = cm.createAlliance("Alliance1");
    Alliance& a2 = cm.createAlliance("Alliance2");

    Colony& c1 =
        cm.createColony(*p1.get(), 0xffff00ff, std::optional<std::reference_wrapper<Alliance>>{a1});
    Colony& c2 =
        cm.createColony(*p2.get(), 0xff0000ff, std::optional<std::reference_wrapper<Alliance>>{a2});
    Colony& c3 =
        cm.createColony(*p3.get(), 0xff00ffff, std::optional<std::reference_wrapper<Alliance>>{a1});

    cm.setAllianceDiplomacy(c1.getAlliance(), c2.getAlliance(), DiplomacyStatus::Enemy);
    ASSERT_EQ(DiplomacyStatus::Enemy, cm.getDiplomacy(c1, c2));
    ASSERT_EQ(DiplomacyStatus::Neutral, cm.getDiplomacy(c2, c1));
}

TEST(ColonyManager, TestIfAllianceChangesBoth)
{
    ColonyManager cm;
    PlayerManager pm;

    TerrainFile tf{1,1};
    Terrain t{tf};

    auto p1 = std::make_unique<DummyPlayer>(
        pm, t, "Test1", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p2 = std::make_unique<DummyPlayer>(
        pm, t, "Test2", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p3 = std::make_unique<DummyPlayer>(
        pm, t, "Test3", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });

    Alliance& a1 = cm.createAlliance("Alliance1");
    Alliance& a2 = cm.createAlliance("Alliance2");

    Colony& c1 =
        cm.createColony(*p1.get(), 0xffff00ff, std::optional<std::reference_wrapper<Alliance>>{a1});
    Colony& c2 =
        cm.createColony(*p2.get(), 0xff0000ff, std::optional<std::reference_wrapper<Alliance>>{a2});
    Colony& c3 =
        cm.createColony(*p3.get(), 0xff00ffff, std::optional<std::reference_wrapper<Alliance>>{a1});

    cm.setAllianceDiplomacy(c1.getAlliance(), c2.getAlliance(), DiplomacyStatus::Ally);
    cm.setAllianceDiplomacy(c2.getAlliance(), c1.getAlliance(), DiplomacyStatus::Ally);
    ASSERT_EQ(DiplomacyStatus::Ally, cm.getDiplomacy(c1, c2));
    ASSERT_EQ(DiplomacyStatus::Ally, cm.getDiplomacy(c2, c1));
}

TEST(ColonyManager, TestIfAllianceChangesAndChangesBack)
{
    ColonyManager cm;
    PlayerManager pm;

    TerrainFile tf{1,1};
    Terrain t{tf};

    auto p1 = std::make_unique<DummyPlayer>(
        pm, t, "Test1", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p2 = std::make_unique<DummyPlayer>(
        pm, t, "Test2", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });
    auto p3 = std::make_unique<DummyPlayer>(
        pm, t, "Test3", 1, [&](size_t) -> std::vector<PlayerInputType> { return {}; });

    Alliance& a1 = cm.createAlliance("Alliance1");
    Alliance& a2 = cm.createAlliance("Alliance2");

    Colony& c1 =
        cm.createColony(*p1.get(), 0xffff00ff, std::optional<std::reference_wrapper<Alliance>>{a1});
    Colony& c2 =
        cm.createColony(*p2.get(), 0xff0000ff, std::optional<std::reference_wrapper<Alliance>>{a2});
    Colony& c3 =
        cm.createColony(*p3.get(), 0xff00ffff, std::optional<std::reference_wrapper<Alliance>>{a1});

    cm.setAllianceDiplomacy(c1.getAlliance(), c2.getAlliance(), DiplomacyStatus::Enemy);
    cm.setAllianceDiplomacy(c2.getAlliance(), c1.getAlliance(), DiplomacyStatus::Enemy);
    ASSERT_EQ(DiplomacyStatus::Enemy, cm.getDiplomacy(c1, c2));
    ASSERT_EQ(DiplomacyStatus::Enemy, cm.getDiplomacy(c2, c1));

    cm.setAllianceDiplomacy(c1.getAlliance(), c2.getAlliance(), DiplomacyStatus::Neutral);

    ASSERT_EQ(DiplomacyStatus::Neutral, cm.getDiplomacy(c1, c2));
    ASSERT_EQ(DiplomacyStatus::Enemy, cm.getDiplomacy(c2, c1));
}
