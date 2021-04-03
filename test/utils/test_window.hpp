#pragma once
/**
 * The classes below mocks a window
 *
 * They are useful to test some aspects of the graphical system without
 * calling any specific API
 */

#include <client/graphical/window.hpp>
#include <client/input/input_service.hpp>
#include <string>
#include <unordered_map>

#include "test_gui_manager.hpp"
#include "test_renderer.hpp"

class TestWindow : public familyline::graphics::Window
{
private:
    TestRenderer* tr = nullptr;
    TestGUIManager* gm_ = nullptr;
    
public:
    virtual void getSize(int& w, int& h) const
    {
        w = 800;
        h = 600;
    }

    /* Get the window framebuffer size
     *
     * This value can differ from the window size if the
     * OS do some sort of dpi scaling
     *
     * macOS is one example
     */
    virtual void getFramebufferSize(int& width, int& height) { this->getSize(width, height); }

    virtual void show() {}
    virtual void setFramebuffers(
        familyline::graphics::Framebuffer* f3D, familyline::graphics::Framebuffer* fGUI)
    {
    }
    virtual void update() {}

    virtual ~TestWindow()
    {
        if (tr) delete tr;
        if (gm_) delete gm_;
    }

    virtual familyline::graphics::Renderer* createRenderer()
    {
        tr = new TestRenderer{};
        return this->getRenderer();
    }
    virtual familyline::graphics::Renderer* getRenderer()
    {
        return (familyline::graphics::Renderer*)tr;
    }

    virtual familyline::graphics::gui::GUIManager* createGUIManager()
    {
        auto& ima = familyline::input::InputService::getInputManager();
        gm_ = new TestGUIManager{*this, 800, 600, *ima.get()};
        return gm_;
    }

    virtual void showMessageBox(
        std::string title, familyline::graphics::SysMessageBoxFlags flags, std::string content)
        {}
};
