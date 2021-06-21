#pragma once

/**
 * Player enumeration routines
 *
 * To start a game, first we need to tell the player managers about
 * what players we have in our system
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <common/logic/player_session.hpp>
#include <tuple>

namespace familyline
{
/**
 * Player information
 *
 * Currently, only the name.
 * Not even the color will appear
 */
struct InitPlayerInfo {
    std::string name;
    uint64_t id = -1;  /// used only for storing the ID from the player manager
};

/**
 * Create a single player player manager.
 *
 * In the future, this will add the AI code to the player manager,
 * but now, only the dummy player (see the cpp file) and the
 * human player will be added.
 */
logic::PlayerSession initSinglePlayerSession(logic::Terrain& terrain, InitPlayerInfo& human_info);

}  // namespace familyline
