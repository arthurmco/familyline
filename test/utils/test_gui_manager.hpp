#pragma once
/**
 * The classes below mocks a GUI manager
 *
 * They are useful to test some aspects of the graphical system without
 * calling any specific API
 */

#include <client/graphical/gui/gui_manager.hpp>

class TestGUIManager : public familyline::graphics::gui::GUIManager
{
public:
    TestGUIManager(
        familyline::graphics::Window& win, unsigned width, unsigned height,
        familyline::input::InputManager& manager)
        : GUIManager(win, width, height, manager)
        {}
    
    virtual void init(const familyline::graphics::Window& win) {}

    virtual void renderToTexture() {}

    virtual ~TestGUIManager() {}
};
