#pragma once

/**
 * Input processor
 * Processes SDL inputs, and convert them to actions, so
 * we do not have to fill the code with SDL-specific functions.
 *
 * However, we retain SDL keycode constants, because it is easier
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <queue>
#include <thread>
#include <string>

#include "input_actions.hpp"

namespace familyline::input
{
/**
 * Processes input from SDL and generate input actions
 *
 * Usually will not generate events that have something to do with game
 * objects, like BuildAction or ObjectClickAction, but you can enable
 * it when the game starts
 */
class InputProcessor
{
private:
    //        InputPicker& _ip;

    std::queue<HumanInputAction> _actions;

    std::thread _runThread;
    bool _isRunning = false;

    void enqueueEvent(const SDL_Event& e, int& lastX, int& lastY);

public:
    //        InputProcessor(InputPicker& ip)
    //            : _ip(ip)
    //            {}

    /**
     * Starts a thread to receive input, so we do not lose input
     * even if the primary thread hangs
     *
     * We might not be able to process it, but the system will never
     * treat this game as non-responding.
     */
    void startInputReceiver();

    /**
     * Stops the input receiver thread
     */
    void stopInputReceiver();

    /**
     * Get the next action
     *
     * Return true if we have a next action, false if the action queue is
     * empty
     */
    bool pollAction(HumanInputAction& a);

    /**
     * Start receiving text events
     *
     * Call this if, for example, you are inserting text in a
     * textbox
     */
    void enableTextEvents();

    /**
     * Stop receiving text events
     *
     * Call this if, for example, you moved focus from a combobox to
     * another control
     */
    void disableTextEvents();

    std::string getClipboardText();
    
    ~InputProcessor() { this->stopInputReceiver(); }
};

}  // namespace familyline::input
