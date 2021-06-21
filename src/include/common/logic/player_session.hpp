
#pragma once

/**
 * Common player session structure
 *
 * (C) 2020 Arthur Mendes
 */

#include <common/logic/colony_manager.hpp>
#include <common/logic/player_manager.hpp>
#include <memory>

namespace familyline::logic
{
struct PlayerSession {
    std::unique_ptr<logic::PlayerManager> players;
    std::unique_ptr<logic::ColonyManager> colonies;

    std::map<uint64_t /*player_id*/, std::reference_wrapper<logic::Colony>> player_colony;
};

}  // namespace familyline::logic
