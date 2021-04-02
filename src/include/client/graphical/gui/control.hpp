#pragma once

#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#include <array>
#include <client/graphical/gui/gui_container_component.hpp>
#include <client/input/input_actions.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <tuple>
#include <vector>

namespace familyline::graphics::gui
{

class Control;

typedef std::function<void(Control*)> EventCallbackFn;

// TODO: make callbacks async?
struct CallbackQueueElement {
    EventCallbackFn fn;
    Control* owner;

    // We use the owner ID to check if the control still
    // exists. If it did not exist anymore, we do not run the
    // callback.
    unsigned owner_id;

    CallbackQueueElement(EventCallbackFn fn, Control* o, unsigned oid)
        : fn(fn), owner(o), owner_id(oid)
    {
    }
};

struct CallbackQueue {
    std::queue<CallbackQueueElement> callbacks;
};



/**
 * Font weight.
 *
 * Those font weight values mimic those of Pango
 * \see https://developer.gnome.org/pango/stable/pango-Fonts.html#PangoWeight
 *
 * Note that not all weight values might be available for all fonts.
 */
enum class FontWeight
{
    Thin,
    Ultralight,
    Light,
    Semilight,
    Book,
    Normal,
    Medium,
    Semibold,
    Bold,
    Ultrabold,
    Heavy,
    Ultraheavy
};

/**
 * Defines the visual appearance of the controls
 *
 * In this structure are all types of visual customization I could think of.
 * Not all customizations will apply to all controls.
 */
struct ControlAppearance {
    std::string fontFace;
    int fontSize;  /// font size, in points
    FontWeight fontWeight = FontWeight::Normal;
    bool italic           = false;

    std::array<double, 4> foreground = {1.0, 1.0, 1.0, 1.0};
    std::array<double, 4> background = {0.0, 0.0, 0.0, 0.0};

    std::array<double, 4> borderColor = {0.0, 0.0, 0.0, 0.0};
};

/**
 * Base control class
 */
class Control
{

public:
    Control();

    unsigned long getID() const { return id_; }

    const ControlAppearance& getAppearance() { return appearance_; }
    virtual void setAppearance(ControlAppearance& a) { appearance_ = a; }

    void modifyAppearance(std::function<void(ControlAppearance&)> fn)
    {
        auto a = this->getAppearance();
        fn(a);
        this->setAppearance(a);
    }

    void updatePosition(int x, int y)
    {
        x_ = x;
        y_ = y;
    }

    /**
     * The control z-index value
     *
     * Bigger the number, more priority it has, more on front of anything
     * else it will appear
     */
    int z_index = 1;

    /**
     * The parent component calls this if it thinks that this control needs to update
     *
     * Return true to signalize that the control updated the framebuffer, false that it
     * did not, and the parent control can reuse caches
     */
    virtual bool update(cairo_t* context, cairo_surface_t* canvas) = 0;

    /**
     * How much width and height, in pixels, your control will need
     *
     * This will be used as a guide to create the surface that will be
     * passed to this component on the `update()` function
     *
     * We pass the parent context just because some controls might need a context,
     * and we do not have ours ready.
     */
    virtual std::tuple<int, int> getNeededSize(cairo_t* parent_context) const = 0;

    /**
     * This is called by the control component when an object is added to it, so
     * you can create a context for it
     */
    virtual std::tuple<cairo_t*, cairo_surface_t*> createChildContext(Control* c);

    /**
     * Set the resize callback, to notify the parent that you resized
     */
    void setResizeCallback(std::function<void(Control*, size_t, size_t)> cb) { resize_cb_ = cb; }

    /**
     * Resize the control
     */
    void resize(size_t w, size_t h);

    std::optional<ContainerComponent>& getControlContainer() { return cc_; }

    virtual void enqueueCallback(CallbackQueue& cq, EventCallbackFn ec);

    virtual void receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq) = 0;

    virtual void onFocusEnter() {}
    virtual void onFocusLost() {}
    
    // see https://stackoverflow.com/a/461224
    virtual ~Control() {}

private:
    unsigned long long id_;
    std::function<void(Control*, size_t, size_t)> resize_cb_;

protected:
    std::optional<ContainerComponent> cc_ = std::nullopt;
    ControlAppearance appearance_;

    int x_;
    int y_;

};

}  // namespace familyline::graphics::gui
