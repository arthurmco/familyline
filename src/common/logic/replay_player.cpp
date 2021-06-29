#include <common/logic/replay_player.hpp>

using namespace familyline::logic;

void ReplayPlayer::enqueueAction(PlayerInputAction a) { actions_.push(a); }

/**
 * Generate the input actions.
 *
 * They must be pushed to the input manager
 */
void ReplayPlayer::generateInput() {
    auto tick = this->getTick();

    while (!actions_.empty()) {
        auto action = actions_.front();
        if (action.tick > tick)
            break;
        
        this->pushAction(action.type);
        actions_.pop();
    }

    if (actions_.empty()) {
        end_callback_(this);
    }
}
