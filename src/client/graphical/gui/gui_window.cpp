#include <client/graphical/gui/gui_window.hpp>

using namespace familyline::graphics::gui;
using namespace familyline::input;

bool GUIWindow::update(cairo_t* context, cairo_surface_t* canvas)
{
    auto [br, bg, bb, ba] = this->appearance_.background;

    cairo_set_source_rgba(context, br, bg, bb, ba);
    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_paint(context);

    bool ret = rc_.update(rc_context_, rc_canvas_);
    
    cairo_set_operator(context, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(context, rc_canvas_, 0, 0);
    cairo_paint(context);
    return ret;
}

void GUIWindow::receiveEvent(const HumanInputAction& hia, CallbackQueue& cq)
{
    HumanInputAction ev = hia;
    
    if (std::holds_alternative<ClickAction>(ev.type)) {
        auto ca = std::get<ClickAction>(ev.type);
        ca.screenX -= x_;
        ca.screenY -= y_;
        ev.type = ca;

        rc_.receiveEvent(ev, cq);
        return;
    }

    if (std::holds_alternative<MouseAction>(ev.type)) {
        auto ma = std::get<MouseAction>(ev.type);
        ma.screenX -= x_;
        ma.screenY -= y_;
        ev.type = ma;       

        rc_.receiveEvent(ev, cq);
        return;
    }

    rc_.receiveEvent(hia, cq);
}
