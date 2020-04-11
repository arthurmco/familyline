#pragma once

/**
 * Manages players and player input events
 *
 * Copyright (C) 2020 Arthur M.
 */

#include <vector>
#include <queue>
#include <memory>
#include <functional>

#include <common/logic/player.hpp>
#include <common/logic/player_actions.hpp>

namespace familyline::logic {

    struct PlayerInfo {
        int id;
        std::unique_ptr<Player> player;

        PlayerInfo(int id, std::unique_ptr<Player> player)
            : id(id), player(std::move(player)) {}
    };
    
    using PlayerListenerHandler = std::function<bool(PlayerInputAction)>;

    struct PlayerHandlerInfo {
        int id;
        PlayerListenerHandler handler;
    };

    class PlayerManager {
    private:
        std::vector<PlayerInfo> players_;

        std::queue<PlayerInputAction> actions_;
        std::vector<PlayerHandlerInfo> player_input_listeners_;

        unsigned int _tick = 0;
        
    public:
        /**
         * Add a player here
         * 
         * Return its generated ID
         */
        int add(std::unique_ptr<Player> p);

        /**
         * Push an action
         */
        void pushAction(int id, PlayerInputType type);

        /**
         * Adds a listener to the player input action event listeners
         *
         * Returns the ID
         */
        int addListener(PlayerListenerHandler h);

        /**
         * Run the input handlers and pop the event from the input action
         * queue
         */
        void run(unsigned int tick);
    };
    
}
