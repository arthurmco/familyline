#pragma once

/**
 * A player whose input comes from the network
 *
 * Copyright (C) 2021 Arthur Mendes
 */

#include <common/logic/player.hpp>
#include <common/net/network_client.hpp>

namespace familyline::net
{

/**
 * The network player
 *
 * In a multiplayer game, is the other human players connected
 * from the network
 *
 * This class will simply get the inputs and convert them into a
 * input action.
 */
class NetworkPlayer : public familyline::logic::Player
{
public:
    NetworkPlayer(logic::PlayerManager& pm, const logic::Terrain& terr, NetworkClient& client)
        : Player(pm, terr, "Network", client.id()), client_(client)
    {
    }

    virtual void generateInput();

private:
    NetworkClient& client_;
};

}  // namespace familyline::net
