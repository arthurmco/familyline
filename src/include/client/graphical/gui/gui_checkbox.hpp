#pragma once

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <client/graphical/gui/control.hpp>
#include <memory>
#include <string>
#include <vector>
#include <client/graphical/gui/gui_label.hpp>

namespace familyline::graphics::gui
{
/**
 * The checkbox GUI control
 */
class Checkbox : public Control
{
private:
    unsigned width_, height_;
    bool enabled_;

    unsigned label_width_, label_height_;
    cairo_t *l_context_        = nullptr;
    cairo_surface_t *l_canvas_ = nullptr;
    Label label_;
    
    PangoLayout* layout_ = nullptr;
    
    cairo_t* last_conlabel_ = nullptr;

    // checkbox size, in pixels
    int checkboxSize_ = 16;
    int checkboxMargin_ = 8;
    
public:
    Checkbox(unsigned width, unsigned height, std::string text, bool enabled=false)
        : width_(width), height_(height), enabled_(enabled), label_(Label{width, height, text})
    {
        label_.setResizeCallback([&](Control* c, size_t w, size_t h) {
            label_width_  = w;
            label_height_ = h;

            cairo_surface_destroy(l_canvas_);
            cairo_destroy(l_context_);

            l_canvas_  = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
            l_context_ = cairo_create(l_canvas_);
        });


        ControlAppearance ca = label_.getAppearance();
        ca.fontFace          = "Arial";
        ca.fontSize          = checkboxSize_;
        ca.foreground        = {1.0, 1.0, 1.0, 1.0};

        label_.setAppearance(ca);

        l_canvas_  = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        l_context_ = cairo_create(l_canvas_);
        label_.update(l_context_, l_canvas_);
        label_.setText(text);

    }

    virtual bool update(cairo_t* context, cairo_surface_t* canvas);

    bool getState() const { return enabled_; }
    
    virtual std::tuple<int, int> getNeededSize(cairo_t* parent_context) const;

    void setText(std::string v);

    virtual void receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq);

    virtual ~Checkbox() {
        if (layout_)
            g_object_unref(layout_);
    }
};

}  // namespace familyline::graphics::gui
