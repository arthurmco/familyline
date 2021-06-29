#include <chrono>
#include <client/graphical/gui/gui_button.hpp>

using namespace familyline::graphics::gui;
using namespace familyline::input;

Button::Button(unsigned width, unsigned height, std::string text)
    : width_(width), height_(height), label_(Label{1, 1, ""})
{
    // We set the text after we set the resize callback, so we can get the
    // text size correctly already when we build the button class
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
    ca.fontSize          = 20;
    ca.foreground        = {1.0, 1.0, 1.0, 1.0};

    label_.setAppearance(ca);

    ca.background  = {0.0, 0.0, 0.0, 0.5};
    ca.borderColor = {0.0, 0.0, 0.0, 1.0};
    appearance_    = ca;

    l_canvas_  = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    l_context_ = cairo_create(l_canvas_);
    label_.update(l_context_, l_canvas_);
    label_.setText(text);
}

bool Button::update(cairo_t* context, cairo_surface_t* canvas)
{
    size_t label_x = width_ / 2 - label_width_ / 2;
    size_t label_y = height_ / 2 - label_height_ / 2;

    auto [br, bg, bb, ba]     = this->appearance_.background;
    auto [bor, bog, bob, boa] = this->appearance_.borderColor;

    // draw the background
    if (clicked_ ||
        std::chrono::steady_clock::now() - last_click_ < std::chrono::milliseconds(100)) {
        cairo_set_source_rgba(context, br, bg, bb, ba * 4);
    } else if (hovered_) {
        cairo_set_source_rgba(context, br, bg, bb, ba * 2);
    } else {
        cairo_set_source_rgba(context, br, bg, bb, ba);
    }

    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_paint(context);

    // draw a border
    cairo_set_line_width(context, 6);
    cairo_set_source_rgba(context, bor, bog, bob, boa);
    cairo_rectangle(context, 0, 0, width_, height_);
    cairo_stroke(context);

    label_.update(l_context_, l_canvas_);

    cairo_set_operator(context, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(context, l_canvas_, label_x, label_y);
    cairo_paint(context);

    return true;
}

std::tuple<int, int> Button::getNeededSize(cairo_t* parent_context) const
{
    return std::tie(width_, height_);
}

void Button::receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq)
{
    if (std::holds_alternative<ClickAction>(ev.type)) {
        auto ca  = std::get<ClickAction>(ev.type);
        clicked_ = ca.isPressed;
    }

    if (clicked_) {
        click_active_ = true;
        this->enqueueCallback(cq, click_cb_);

        /// BUG: if the line below is uncommented, the game crashes, probably due to some
        /// cross-thread access.
        ///
        /// Maybe make the GUI run in its own thread, and run the callbacks on the main thread.

        // click_fut_ = std::async(this->click_cb_, this);
        clicked_ = false;
    }

    // Separating the click action (the clicked_ variable) from the "is clicked" question
    // allows us to set the last click only when the user releases the click. This is a better
    // and more reliable way than activating it on the button down event, or when the clicked_
    // is false
    if (click_active_) {
        if (!clicked_) {
            last_click_   = std::chrono::steady_clock::now();
            click_active_ = false;
        }
    }
}

void Button::setText(std::string v) { label_.setText(v); }

Button::~Button()
{
    cairo_surface_destroy(l_canvas_);
    cairo_destroy(l_context_);
}
