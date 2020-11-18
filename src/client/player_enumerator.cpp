
#include <client/HumanPlayer.hpp>
#include <client/player_enumerator.hpp>
#include <common/logger.hpp>
#include <common/logic/terrain.hpp>

using namespace familyline;
using namespace familyline::logic;

class DummyPlayer : public Player
{
private:
    bool done         = false;
    size_t sync_start = 0;
    size_t input_tick = 0;

    void pushActions(std::initializer_list<PlayerInputType> input)
    {
        for (auto& i : input) {
            this->pushAction(i);
        }
    }

public:
    DummyPlayer(PlayerManager& pm, const Terrain& t, const char* name, int code)
        : Player(pm, t, name, code)
    {
    }

    virtual void generateInput()
    {
        if (done) return;

        auto tick = this->getTick();

        if (sync_start == 0) {
            if (this->isTickValid()) {
                sync_start = tick;
                printf("engine is synchronized at tick %zu", sync_start);
            } else {
                return;
            }
        }

        if (input_tick == 2) {
            this->pushActions({EnqueueBuildAction{"tent"}});
        }

        if (input_tick == 16) {
            this->pushActions(
                {CommitLastBuildAction{20.0, 20.0, 2.41, true}, EnqueueBuildAction{"tent"}});
        }

        if (input_tick == 310) {
            this->pushActions({
                CommitLastBuildAction{30.0, 35.0, 2.41, true},
            });

            puts("Dummy player built everything it needed");
            done = true;
        }

        input_tick++;
    }
};

std::unique_ptr<logic::PlayerManager> initPlayerManager(
    logic::Terrain& terrain, InitPlayerInfo& human_info)
{
    auto& log = LoggerService::getLogger();
    log->write("player_enumerator", LogType::Info, "adding human and the dummy player");

    auto pm = std::make_unique<PlayerManager>();

    auto hid = pm->add(
        std::unique_ptr<Player>(new HumanPlayer{*pm.get(), terrain, human_info.name.c_str(), 0}));
    pm->add(std::unique_ptr<Player>(new DummyPlayer{*pm.get(), terrain, "Dummy Player", 1}));

    fprintf(stderr, "%x ----", hid);

    human_info.id = hid;
    return std::move(pm);
}

/**
 * Create a colony
 *
 * For now, every player is a 'neutral' of the other, (aka will not
 * attack you if you wont attack me), but this will change
 *
 * We also fill the player_colony map with the mapping of player->colony
 * we find.
 */
std::unique_ptr<logic::ColonyManager> initColonyManager(
    PlayerManager& pm,
    std::map<unsigned int, std::reference_wrapper<logic::Colony>>& player_colony
    ) {

    auto playerlist = pm.getPlayerNames();
    auto cm = std::make_unique<ColonyManager>();
    
    puts("aaa");
    for (auto [id, name] : playerlist) {
        // generate a random color
        // we will see the mess when the game starts showing this color
        long long int colorbase = (id & 0xffffff) + (id >> 8);        
        int color = int(colorbase & 0xffffff);
        
        auto* player = *pm.get(id);
        auto& alliance = cm->createAlliance(name);
        auto& colony = cm->createColony(*player, color, std::optional<std::reference_wrapper<Alliance>>{alliance});

        player_colony.emplace(id, std::reference_wrapper(colony));
        
        int cr = (color >> 16) & 0xff;
        int cg = (color >> 8) & 0xff;
        int cb = color & 0xff;
        
        printf("\033[38;2;%d;%d;%dm %x => %s \033[0m", cr, cg, cb,
               id, name.c_str());
        
    }
    puts("");

    return cm;
}

/**
 * Create a single player player manager.
 *
 * In the future, this will add the AI code to the player manager,
 * but now, only the dummy player (see the cpp file) and the
 * human player will be added.
 *
 * Returns the player manager, and sets the ID of the human_info parameter
 * to the human player ID
 */
PlayerSession familyline::initSinglePlayerSession(
    logic::Terrain& terrain, InitPlayerInfo& human_info)
{

    std::map<unsigned int /*player_id*/, std::reference_wrapper<logic::Colony>> player_colony;
    
    auto pm = initPlayerManager(terrain, human_info);
    auto cm = initColonyManager(*pm.get(), player_colony);

    return PlayerSession{
        std::move(pm),
        std::move(cm),
        player_colony
    };
}

// TODO: how to fill the colony manager?