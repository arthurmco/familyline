#include <GL/glew.h>

#include <client/graphical/exceptions.hpp>
#include <client/graphical/gfx_service.hpp>
#include <client/graphical/gui/gui_manager.hpp>
#include <client/graphical/window.hpp>

using namespace familyline::graphics::gui;
using namespace familyline::graphics;
using namespace familyline::input;

static int vv = 0;

/**
 * Show the window you pass
 *
 * Since the window have a "hidden" delete event, you do not need to remove the
 * window from the gui, deallocating it will be sufficient
 */
void GUIManager::showWindow(GUIWindow* win)
{
    win->event_onDelete = [&](GUIWindow& w) { this->closeWindow(w); };

    auto [w, h] = win->getNeededSize(context_);

    auto* canvas  = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    auto* context = cairo_create(canvas);
    windowstack_.push_back(GUIWindowInfo{win, context, canvas});
}

void GUIManager::closeWindow(const GUIWindow& w)
{
    if (!destroying) {
        auto itend = std::remove_if(windowstack_.begin(), windowstack_.end(), [&](GUIWindowInfo& wi) {
            return (wi.win->getID() == w.getID());
        });
        windowstack_.erase(itend, windowstack_.end());
    }
}

void GUIManager::update()
{
    root_control_->update(context_, canvas_);

    // Clean bg
    cairo_set_source_rgba(context_, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(context_, CAIRO_OPERATOR_SOURCE);
    cairo_paint(context_);

    auto w = cairo_image_surface_get_width(canvas_);
    auto h = cairo_image_surface_get_height(canvas_);

    auto dabsx = 0;
    auto dabsy = 0;
    debug_window_info_.win->update(debug_window_info_.context, debug_window_info_.canvas);

    for (auto& win : windowstack_) {
        win.win->update(win.context, win.canvas);
        cairo_set_operator(context_, CAIRO_OPERATOR_OVER);

        auto absx = 0;
        auto absy = 0;

        cairo_set_source_surface(context_, win.canvas, absx, absy);
        win.win->updatePosition(absx, absy);
        cairo_paint(context_);
    }

    cairo_set_operator(context_, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(context_, debug_window_info_.canvas, dabsx, dabsy);
    debug_window_info_.win->updatePosition(dabsx, dabsy);

    cairo_paint(context_);
}

void GUIManager::render(unsigned int x, unsigned int y) { this->renderToTexture(); }

GUIManager::~GUIManager()
{
    destroying = true;
    windowstack_.clear();
    cairo_destroy(context_);
    cairo_surface_destroy(canvas_);

    // TODO: remove the input handler
}

/**
 * Get the control that is in the specified pixel coordinate
 *
 * Since we only need to worry about windows, we can simplify this.
 */
std::optional<Control*> GUIManager::getControlAtPoint(int x, int y)
{
    if (windowstack_.empty())
        return std::nullopt;
    else
        return std::optional<Control*>((Control*)windowstack_.back().win);
}

bool GUIManager::checkIfEventHits(const HumanInputAction& hia)
{
    if (std::holds_alternative<GameExit>(hia.type)) {
        return false;
    }

    if (std::holds_alternative<ClickAction>(hia.type)) {
        auto ca = std::get<ClickAction>(hia.type);
        return this->getControlAtPoint(ca.screenX, ca.screenY).has_value();
    }

    if (std::holds_alternative<MouseAction>(hia.type)) {
        auto ma    = std::get<MouseAction>(hia.type);
        hitmousex_ = ma.screenX;
        hitmousey_ = ma.screenY;
        return this->getControlAtPoint(ma.screenX, ma.screenY).has_value();
    }

    if (std::holds_alternative<KeyAction>(hia.type)) {
        return this->getControlAtPoint(hitmousex_, hitmousey_).has_value();
    }

    if (std::holds_alternative<WheelAction>(hia.type)) {
        auto wa = std::get<WheelAction>(hia.type);
        return this->getControlAtPoint(wa.screenX, wa.screenY).has_value();
    }

    if (std::holds_alternative<TextInput>(hia.type)) {
        auto ma = std::get<TextInput>(hia.type);
        return this->getControlAtPoint(hitmousex_, hitmousey_).has_value();
    }

    return false;
}

auto mousex_ = -1;
auto mousey_ = -1;

/**
 * Process received input events
 *
 */
void GUIManager::receiveEvent()
{
    while (!input_actions_.empty()) {
        auto& hia           = input_actions_.front();
        bool is_mouse_event = false;

        std::optional<Control*> control = std::nullopt;

        if (std::holds_alternative<ClickAction>(hia.type)) {
            auto ca = std::get<ClickAction>(hia.type);
            control = this->getControlAtPoint(ca.screenX, ca.screenY);
        }

        if (std::holds_alternative<MouseAction>(hia.type)) {
            auto ma        = std::get<MouseAction>(hia.type);
            mousex_        = ma.screenX;
            mousey_        = ma.screenY;
            is_mouse_event = true;
            control        = this->getControlAtPoint(ma.screenX, ma.screenY);
        }

        if (std::holds_alternative<KeyAction>(hia.type)) {
            auto ka = std::get<KeyAction>(hia.type);
            if (mousex_ >= 0) control = this->getControlAtPoint(mousex_, mousey_);
        }

        if (std::holds_alternative<WheelAction>(hia.type)) {
            auto wa = std::get<WheelAction>(hia.type);
            control = this->getControlAtPoint(wa.screenX, wa.screenY);
        }

        if (std::holds_alternative<TextInput>(hia.type)) {
            auto ma = std::get<TextInput>(hia.type);
            control = this->getControlAtPoint(hitmousex_, hitmousey_);
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

            (*control)->receiveEvent(hia, cb_queue_);
        }

        input_actions_.pop();
    }
}

void GUIManager::runCallbacks()
{
    if (!cb_queue_.callbacks.empty()) {
        // Run one callback per call.
        auto cb = cb_queue_.callbacks.front();
        cb_queue_.callbacks.pop();

        // TODO: check if the owner exists.
        cb.fn(cb.owner);
    }
}

GUIWindow* GUIManager::createGUIWindow(std::string name, unsigned width, unsigned height)
{
    if (windows_.find(name) == windows_.end()) {
        windows_[name] = std::make_unique<GUIWindow>(width, height);
    }

    return windows_[name].get();
}

GUIWindow* GUIManager::getGUIWindow(std::string name)
{
    if (auto it = windows_.find(name); it != windows_.end()) {
        return it->second.get();
    }

    return nullptr;
}

/**
 * Destroy window
 */
void GUIManager::destroyGUIWindow(std::string name)
{
    if (auto it = windows_.find(name); it != windows_.end()) {
        /**
         * If the current window is being hovered, "unhover" it, so
         * that a dangling pointer does not remain
         */
        if (hovered_) {
            if ((*hovered_)->getID() == it->second->getID()) hovered_ = std::nullopt;
        }

        windows_.erase(it);
    }
}

////////////////////////////////////////////////
