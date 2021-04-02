#pragma once

/**
 * Manages players and player input events
 *
 * Copyright (C) 2020 Arthur M.
 */

#include <common/logic/PathFinder.hpp>
#include <common/logic/lifecycle_manager.hpp>
#include <common/logic/player.hpp>
#include <common/logic/player_actions.hpp>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace familyline::logic
{
struct PlayerInfo {
    int id;
    std::unique_ptr<Player> player;

    PlayerInfo(int id, std::unique_ptr<Player> player) : id(id), player(std::move(player)) {}
};

using PlayerListenerHandler = std::function<bool(PlayerInputAction)>;

struct PlayerHandlerInfo {
    int id;
    PlayerListenerHandler handler;
};

using PlayerCallback = std::function<void(Player*)>;

/**
 * The player manager
 *
 * It will manage the player instances and execute its actions.
 * Since some clients might not be able to send actions in time (because of network),
 * we need to make the tick we are executing different from the tick that the pushed message
 * will be run
 */
class PlayerManager
{
private:
    std::vector<PlayerInfo> players_;

    /**
     * A "queue" of actions
     *
     * Not a queue, really, because we might want to sort it
     */
    std::deque<PlayerInputAction> actions_;

    std::vector<PlayerHandlerInfo> player_input_listeners_;

    /// The tick we are executing right now
    size_t tick_ = 0;

    /// The difference between the tick we are executing and the tick that the message pushed in
    /// this tick will be run by default
    size_t tick_delta_ = 2;

    void processAction(const PlayerInputAction& a, ObjectManager& om);

    std::optional<Player*> getPlayerFromID(int id);

public:
    ObjectLifecycleManager* olm = nullptr;
    PathFinder* pf              = nullptr;

    /* Callback to a function that allows us to add
     * an object to the renderer
     */
    std::function<void(std::shared_ptr<GameObject>)> render_add_callback;

    /**
     * Callback to a function that allows us to
     * add an object to a colony
     */
    std::function<void(std::shared_ptr<GameObject>, unsigned /*player_id*/)> colony_add_callback;

    /**
     * Add a player here
     *
     * Return its generated ID
     */
    int add(std::unique_ptr<Player> p, bool allocate_id = true);

    /**
     * Get a player from the player manager
     *
     * Returns the player object.
     * Remember that this object is owned by the player manager.
     * In C++, you are the borrow checker.
     */
    std::optional<Player*> get(int id);

    /**
     * Gets a multimap of ID=>playername, so you can easily discover the
     * ID of a certain player name, or a name of a player who has a
     * certain ID
     */
    std::multimap<int, std::string> getPlayerNames();

    /**
     * Push an action
     *
     * We can push an action to be ran in a certain tick
     */
    void pushAction(
        unsigned int id, PlayerInputType type, std::optional<unsigned int> tick = std::nullopt);

    /**
     * Adds a listener to the player input action event listeners
     *
     * Returns the ID
     */
    int addListener(PlayerListenerHandler h);

    /**
     * Removes the player input action event listener
     */
    void removeListener(int id);

    /**
     * Generate input from all players
     */
    void generateInput();

    /**
     * Exit was requested by some player
     *
     * This can only be requested by the human player.
     */
    bool exitRequested();

    /**
     * Run the input handlers and pop the event from the input action
     * queue
     */
    void run(GameContext& gctx);

    /**
     * Iterate between the players, allows the game interface to iterate on the player
     * list
     */
    void iterate(PlayerCallback c);

    /**
     * Get the current tick, as registered by the player manager
     */
    size_t tick() const { return tick_; }

    /**
     * Get the current tick delta
     */
    size_t tickDelta() const { return tick_delta_; }

    /**
     * TODO: maybe add support to changing the tick delta value
     */
};

}  // namespace familyline::logic
