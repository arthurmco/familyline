#include <algorithm>
#include <client/graphical/gui/control.hpp>
#include <ctime>
#include <numeric>  // for std::accumulate

using namespace familyline::graphics::gui;

Control::Control() { id_ = (uintptr_t)this; }

/**
 * This is called by the control component when an object is added to it, so
 * you can create a context for it
 */
std::tuple<cairo_t*, cairo_surface_t*> Control::createChildContext(Control* c)
{
    auto [w, h]             = c->getNeededSize(nullptr);
    cairo_surface_t* canvas = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t* context        = cairo_create(canvas);

    if (w <= 1 || h <= 1) {
        auto [w, h] = c->getNeededSize(context);

        if (canvas)
            cairo_surface_destroy(canvas);

        if (context)
            cairo_destroy(context);

        canvas  = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
        context = cairo_create(canvas);

    }


    return std::tie(context, canvas);
}

/**
 * Resize the control
 */
void Control::resize(size_t w, size_t h) { resize_cb_(this, w, h); }

void Control::enqueueCallback(CallbackQueue& cq, EventCallbackFn ec)
{
    CallbackQueueElement cqe(ec, this, this->getID());
    cq.callbacks.push(cqe);
}
