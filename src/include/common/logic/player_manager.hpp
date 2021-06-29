#pragma once

/**
 * Manages players and player input events
 *
 * Copyright (C) 2020 Arthur M.
 */

#include <common/logic/lifecycle_manager.hpp>
#include <common/logic/pathfinder.hpp>
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
    uint64_t id;
    std::unique_ptr<Player> player;

    PlayerInfo(uint64_t id, std::unique_ptr<Player> player) : id(id), player(std::move(player)) {}
};

enum PlayerHandlerType { AddHandler, RunHandler };

/**
 * The player listener handler
 *
 * You return true if you used this message, false if you did not
 * used
 */
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

    /// Handlers that are executed when you add an event
    std::vector<PlayerHandlerInfo> player_input_listeners_add_;

    /// Handlers that are executed when you run an event
    std::vector<PlayerHandlerInfo> player_input_listeners_run_;

    /// The tick we are executing right now
    size_t tick_ = 0;

    /// The difference between the tick we are executing and the tick that the message pushed in
    /// this tick will be run by default
    size_t tick_delta_ = 10;

    void processAction(const PlayerInputAction& a, ObjectManager& om);

    std::optional<Player*> getPlayerFromID(uint64_t id);

public:
    ObjectLifecycleManager* olm = nullptr;
    Pathfinder* pf              = nullptr;

    /* Callback to a function that allows us to add
     * an object to the renderer
     */
    std::function<void(std::shared_ptr<GameObject>)> render_add_callback;

    /**
     * Callback to a function that allows us to
     * add an object to a colony
     */
    std::function<void(std::shared_ptr<GameObject>, uint64_t /*player_id*/)> colony_add_callback;

    /**
     * Add a player here
     *
     * Return its generated ID
     */
    uint64_t add(std::unique_ptr<Player> p, bool allocate_id = true);

    /**
     * Get a player from the player manager
     *
     * Returns the player object.
     * Remember that this object is owned by the player manager.
     * In C++, you are the borrow checker.
     */
    std::optional<Player*> get(uint64_t id);

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
        uint64_t id, PlayerInputType type, std::optional<unsigned int> tick = std::nullopt);

    /**
     * Adds a listener to the player input action event listeners
     *
     * Returns the ID
     *
     * Note that, if you add a listener for when an action add event, they might not be
     * in order. The action run event is guaranteed to be in order
     */
    int addListener(PlayerListenerHandler h, PlayerHandlerType type = PlayerHandlerType::RunHandler);

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
