#include <client/graphical/gui/root_control.hpp>

using namespace familyline::graphics::gui;
using namespace familyline::input;

bool RootControl::update(cairo_t* context, cairo_surface_t* canvas)
{
    (void)context;

    // Clean bg
    cairo_set_source_rgba(context, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_paint(context);

    auto w = cairo_image_surface_get_width(canvas);
    auto h = cairo_image_surface_get_height(canvas);

    this->cc_->sortZIndex();

    // Paint all children in the correct place
    for (auto& cdata : this->cc_->children) {
        cdata.control->update(cdata.local_context, cdata.control_canvas);

        cairo_set_operator(context, CAIRO_OPERATOR_OVER);

        switch (cdata.pos_type) {
            case ControlPositioning::Pixel:
                cairo_set_source_surface(context, cdata.control_canvas, cdata.x, cdata.y);
                break;
            case ControlPositioning::Relative: {
                auto absx = w * cdata.fx;
                auto absy = h * cdata.fy;
                cairo_set_source_surface(context, cdata.control_canvas, absx, absy);
                this->cc_->updateAbsoluteCoord(cdata.control->getID(), absx, absy);
                break;
            }
            case ControlPositioning::CenterX: {
                auto ctrlw = cairo_image_surface_get_width(cdata.control_canvas);
                auto absx  = (w / 2) - (ctrlw / 2);
                auto absy  = h * cdata.fy;
                cairo_set_source_surface(context, cdata.control_canvas, absx, absy);
                this->cc_->updateAbsoluteCoord(cdata.control->getID(), absx, absy);
                break;
            }
            case ControlPositioning::CenterY: {
                auto ctrlh = cairo_image_surface_get_height(cdata.control_canvas);
                auto absx  = w * cdata.fx;
                auto absy  = (h / 2) - (ctrlh / 2);
                cairo_set_source_surface(context, cdata.control_canvas, absx, absy);
                this->cc_->updateAbsoluteCoord(cdata.control->getID(), absx, absy);
                break;
            }
            case ControlPositioning::CenterAll: {
                auto ctrlw = cairo_image_surface_get_width(cdata.control_canvas);
                auto ctrlh = cairo_image_surface_get_height(cdata.control_canvas);
                auto absx  = (w / 2) - (ctrlw / 2);
                auto absy  = (h / 2) - (ctrlh / 2);
                cairo_set_source_surface(context, cdata.control_canvas, absx, absy);
                this->cc_->updateAbsoluteCoord(cdata.control->getID(), absx, absy);
                break;
            }
        }

        cairo_paint(context);
    }

    return true;
}

void RootControl::onFocusLost()
{
    (*hovered_)->onFocusLost();

    hovered_ = std::nullopt;
}


void RootControl::receiveEvent(const HumanInputAction& hia, CallbackQueue& cq)
{
    std::optional<Control*> control;
    bool is_mouse_event = false;
    
    if (std::holds_alternative<ClickAction>(hia.type)) {
        auto ca = std::get<ClickAction>(hia.type);
        control = this->cc_->getControlAtPoint(ca.screenX, ca.screenY);
    }

    if (std::holds_alternative<MouseAction>(hia.type)) {
        auto ma = std::get<MouseAction>(hia.type);
        mousex_ = ma.screenX;
        mousey_ = ma.screenY;
        is_mouse_event = true;
        control = this->cc_->getControlAtPoint(ma.screenX, ma.screenY);
    }

    if (std::holds_alternative<KeyAction>(hia.type)) {
        auto ka = std::get<KeyAction>(hia.type);
        control = this->cc_->getControlAtPoint(mousex_, mousey_);
    }

    if (std::holds_alternative<WheelAction>(hia.type)) {
        auto wa = std::get<WheelAction>(hia.type);
        control = this->cc_->getControlAtPoint(wa.screenX, wa.screenY);
    }

    if (std::holds_alternative<TextInput>(hia.type)) {
        auto ma = std::get<TextInput>(hia.type);
        control = this->cc_->getControlAtPoint(mousex_, mousey_);
    }
    
    if (control.has_value()) {
        if (is_mouse_event) {
            if (hovered_) {
                // a control was already focused
                if ((*hovered_)->getID() != (*control)->getID()) {
                    (*hovered_)->onFocusLost();
                        
                    (*control)->onFocusEnter();
                    hovered_ = control;
                }
                
            } else {
                // no control focused yet
                (*control)->onFocusEnter();
                hovered_ = control;
            }
            
        }
        
        (*control)->receiveEvent(hia, cq);
    } else {
        if (hovered_) {
            // a control was already focused
            (*hovered_)->onFocusLost();
            hovered_ = std::nullopt;
        }
    }
}
