#pragma once

/**
 * Part of the implementation for the build action
 *
 * An input action is basically a sum type of all the actions you
 * can do in game
 *
 * We have two:
 *  - the human input action, for things that only humans (read: the one
 *    that can click on the menus) can do, like click, move the mouse
 *  - the player input action, for things that all players can do, like
 *    request objects to be build, and position the camera.
 *
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <SDL2/SDL.h>

#include <cstdint>
#include <optional>
#include <variant>

namespace familyline::input
{
/// Simple Events
struct ClickAction {
    int screenX, screenY;
    int buttonCode;
    int clickCount;
    bool isPressed;
};

struct MouseAction {
    int screenX, screenY;
};

struct KeyAction {
    SDL_Keycode keycode;
    const char* keyname;
    bool isPressed, isRepeated;
    uint16_t modifiers;
};

struct WheelAction {
    int screenX, screenY;
    int scrollX, scrollY;
};

/// Text input event.
///
/// Sent when you type some text
struct TextInput {
    char text[32];
};

/// Text edit event.
///
/// Sent when the system does not registered all input for a character, but
/// decided to show you anyway
struct TextEdit {
    char text[32];
    int start;
    int length;
};

struct GameExit {
    int why;
};

////

using HumanInputType =
    std::variant<ClickAction, MouseAction, KeyAction, WheelAction, TextInput, TextEdit, GameExit>;

struct HumanInputAction {
    uint64_t timestamp;
    HumanInputType type;
};

}  // namespace familyline::input
