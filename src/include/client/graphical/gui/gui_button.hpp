#pragma once

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <client/graphical/gui/control.hpp>
#include <client/graphical/gui/gui_label.hpp>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace familyline::graphics::gui
{
class Button : public Control
{
private:
    unsigned width_, height_;
    Label label_;

    unsigned label_width_, label_height_;
    cairo_t *l_context_        = nullptr;
    cairo_surface_t *l_canvas_ = nullptr;
    EventCallbackFn click_cb_ = [](void* cc){};

    std::chrono::time_point<std::chrono::steady_clock> last_hover_ =
        std::chrono::steady_clock::now();

    bool hovered_ = false;

    bool clicked_      = false;
    bool click_active_ = false;

    /// Stores the last time the user clicked, so we can make the click animation last more
    /// than the milisseconds between the GUI updates, a time sufficient for the user to
    /// see
    std::chrono::time_point<std::chrono::steady_clock> last_click_ =
        std::chrono::steady_clock::now();

    std::future<void> click_fut_;

public:
    virtual void setAppearance(ControlAppearance &a)
    {
        appearance_   = a;
        a.background  = {0, 0, 0, 0};
        a.borderColor = {0, 0, 0, 0};
        label_.setAppearance(a);
    }

    Button(unsigned width, unsigned height, std::string text);

    virtual bool update(cairo_t *context, cairo_surface_t *canvas);

    virtual std::tuple<int, int> getNeededSize(cairo_t *parent_context) const;

    void setClickCallback(EventCallbackFn c) { click_cb_ = c; }

    void setText(std::string v);

    virtual void onFocusEnter() { hovered_ = true; }
    virtual void onFocusLost() { hovered_ = false; }

    
    virtual void receiveEvent(const familyline::input::HumanInputAction &ev, CallbackQueue &cq);

    virtual ~Button();
};

}  // namespace familyline::graphics::gui
