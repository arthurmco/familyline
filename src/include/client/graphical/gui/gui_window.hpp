#pragma once

/**
 * Window GUI control
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <client/graphical/gui/root_control.hpp>
#include <optional>

namespace familyline::graphics::gui
{
/**
 * The window control
 *
 * It is a container for other controls
 */
class GUIWindow : public Control
{
private:
    unsigned width_, height_;

    RootControl rc_;
    cairo_t *rc_context_        = nullptr;
    cairo_surface_t *rc_canvas_ = nullptr;
    
public:
    GUIWindow(unsigned width, unsigned height)
        : width_(width), height_(height), rc_(width_, height_)
    {
        rc_canvas_  = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width_, height_);
        rc_context_ = cairo_create(rc_canvas_);

        this->appearance_.background = {0.0, 0.0, 0.0, 0.8};
    }

    virtual std::tuple<int, int> getNeededSize(cairo_t* parent_context) const
    {
        return std::tie(width_, height_);
    }

    virtual bool update(cairo_t* context, cairo_surface_t* canvas);
    virtual void receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq);

    void add(double x, double y, ControlPositioning cpos, std::unique_ptr<Control> control,
             std::string name="") {
        rc_.getControlContainer()->add(x, y, cpos, std::move(control), name);
    }

    void remove(Control* control) {
        if (control)
            rc_.getControlContainer()->remove(control->getID());
    }

    Control* get(std::string name) {
        return rc_.getControlContainer()->get(name);
    }

    std::function<void(GUIWindow&)> event_onDelete;

    virtual void onFocusLost() { rc_.onFocusLost(); }

    
    virtual ~GUIWindow() {
        if (event_onDelete)
            event_onDelete(*this);

        cairo_surface_destroy(rc_canvas_);
        cairo_destroy(rc_context_);
    }
};

}  // namespace familyline::graphics::gui
