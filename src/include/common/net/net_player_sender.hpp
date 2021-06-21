/**
 * Sends player data to its other peers in a networked game
 *
 * Copyright (C) 2021 Arthur Mendes
 */

#include <common/logic/player_manager.hpp>
#include <functional>

#include "common/net/game_packet_server.hpp"

namespace familyline::net
{
/**
 * This class will listen to messages in the player manager, and forward
 * the ones that come from the player to its peers
 *
 * In the future, it will not forward bot messages, because bots will be
 * created independently.
 */
class NetPlayerSender
{
public:
    NetPlayerSender(logic::PlayerManager& pm, GamePacketServer& gps, uint64_t human_id)
        : human_id_(human_id), gps_(gps), pm_(pm)
    {
        listener_id_ =
            pm.addListener(std::bind(&NetPlayerSender::listenPlayer, this, std::placeholders::_1));
    }

    ~NetPlayerSender() { pm_.removeListener(listener_id_); }

private:
    logic::PlayerManager& pm_;
    GamePacketServer& gps_;
    uint64_t human_id_;
    int listener_id_ = -1;

    bool listenPlayer(logic::PlayerInputAction);
};

}  // namespace familyline::net
