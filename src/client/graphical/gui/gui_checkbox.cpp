
#include <client/graphical/gui/gui_checkbox.hpp>

#include "pango/pango-font.h"

using namespace familyline::graphics::gui;
using namespace familyline::input;


bool Checkbox::update(cairo_t* context, cairo_surface_t* canvas)
{
    auto [fr, fg, fb, fa] = this->appearance_.foreground;
    auto [br, bg, bb, ba] = this->appearance_.background;

    if (layout_) {
        pango_layout_set_font_description(layout_, nullptr);
        g_object_unref(layout_);
    }
    
    cairo_set_source_rgba(context, br, bg, bb, ba);
    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_paint(context);

    // draw the box and, maybe, the X
    cairo_move_to(context, 0, 0);
    cairo_set_line_width(context, checkboxSize_ / 8);
    cairo_set_source_rgba(context, fr, fg, fb, fa);
    cairo_rectangle(context, 0, 0, checkboxSize_, checkboxSize_);
    cairo_stroke(context);

    if (enabled_) {
        cairo_move_to(context, 0, 0);
        cairo_line_to(context, checkboxSize_, checkboxSize_);
        cairo_move_to(context, 0, checkboxSize_);
        cairo_line_to(context, checkboxSize_, 0);
    }    

    label_.update(l_context_, l_canvas_);

    cairo_set_operator(context, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(context, l_canvas_, checkboxSize_ + checkboxMargin_, 0);
    cairo_paint(context);
    
    return true;
}

std::tuple<int, int> Checkbox::getNeededSize(cairo_t* parent_context) const
{
    auto [width, height] = label_.getNeededSize(parent_context);
    width += checkboxSize_ + checkboxMargin_;
    return std::tie(width, height);
}

void Checkbox::receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq) 
{
    if (std::holds_alternative<ClickAction>(ev.type)) {
        auto ca  = std::get<ClickAction>(ev.type);
        if (ca.isPressed && ca.buttonCode == 1) {
            enabled_ = !enabled_;
        }
    }
}


void Checkbox::setText(std::string v)
{
    this->label_.setText(v);
}
