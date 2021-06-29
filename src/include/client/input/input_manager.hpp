#pragma once

/**
 * Manages the user input
 *
 * (C) 2020 Arthur Mendes
 */

#include <common/logic/player_actions.hpp>
#include <functional>
#include <queue>
#include <vector>

#include "input_processor.hpp"

namespace familyline::input
{
using HumanListenerHandler  = std::function<bool(HumanInputAction)>;
using PlayerListenerHandler = std::function<bool(familyline::logic::PlayerInputAction)>;

using listener_handler_t = int;

/**
 * Struct used to implement ordering to the handlers, so we can call them
 * in the correct order, like first the GUI, and then the 3d area, since we
 * do not want to, instead of typing something into the chat, we build some
 * random buildings
 *
 * It is templated so we can use one for both the human and player input listeners
 */
template <typename T>
struct HandlerOrder {
    T handler;
    listener_handler_t id;
    int zindex;
};

/**
 * Manages user input, and forward it to the listeners via a
 * callback
 */
class InputManager
{
private:
    InputProcessor& _ip;

    unsigned _tick = 0;

    std::vector<HandlerOrder<HumanListenerHandler>> _human_input_fns;

    std::queue<familyline::logic::PlayerInputAction> _input_actions;

public:
    InputManager(InputProcessor& ip);


    /**
     * Start receiving text events
     *
     * Call this if, for example, you are inserting text in a
     * textbox
     */
    void enableTextEvents() { _ip.enableTextEvents(); }

    /**
     * Stop receiving text events
     *
     * Call this if, for example, you moved focus from a combobox to
     * another control
     */
    void disableTextEvents() { _ip.disableTextEvents(); }

    std::string getClipboardText() { return _ip.getClipboardText(); }

    
    /**
     * Process input events, human inputs and player inputs
     *
     * The human inputs come from the input processor
     * The player input come from the player who call `queuePlayerInput`
     */
    void processEvents();

    /**
     * Adds a listener handler, returns an ID
     */
    listener_handler_t addListenerHandler(HumanListenerHandler h, int zorder = 0);

    void removeListenerHandler(listener_handler_t);
};

}  // namespace familyline::input
