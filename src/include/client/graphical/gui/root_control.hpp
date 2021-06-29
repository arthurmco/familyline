#pragma once

#include <client/graphical/gui/control.hpp>
#include <optional>

namespace familyline::graphics::gui
{
/**
 * The root control
 *
 * Every single control in the interface is a child of it
 *
 * Manages GUI-specific things on the interface, like caring about
 * repainting children controls, and control ordering, so that
 * the GUI manager do not need to.
 *
 */
class RootControl : public Control
{
    // TODO: define how controls will be positioned: pixel-based, vbox-based (like GTK)?

private:
    unsigned width_, height_;

    int mousex_ = 1, mousey_ = 1;

    /// Control we are hovering at
    std::optional<Control*> hovered_ = std::nullopt;
    
    /**
     * Sort the controls by their z-index values
     */
    void sortZIndex();

public:
    RootControl(unsigned width, unsigned height) : width_(width), height_(height)
    {
        cc_         = std::make_optional<ContainerComponent>();
        cc_->parent = dynamic_cast<Control*>(this);
    }

    virtual bool update(cairo_t* context, cairo_surface_t* canvas);

    virtual std::tuple<int, int> getNeededSize(cairo_t* parent_context) const
    {
        return std::tie(width_, height_);
    }

    virtual void receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq);

    virtual void onFocusLost();
    
    virtual ~RootControl() {}
};

}  // namespace familyline::graphics::gui
