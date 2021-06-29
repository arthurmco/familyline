#include <fmt/format.h>

#include <algorithm>
#include <client/graphical/gui/control.hpp>
#include <client/graphical/gui/gui_container_component.hpp>
#include <ctime>
#include <numeric>  // for std::accumulate

using namespace familyline::graphics::gui;

void ContainerComponent::add(int x, int y, std::unique_ptr<Control> c, std::string name = "")
{
    c->setResizeCallback([&](Control* co, size_t w, size_t h) {
        auto co_it = std::find_if(
            this->children.begin(), this->children.end(),
            [&](ControlData& cd) { return cd.control->getID() == co->getID(); });

        if (co_it != this->children.end()) {
            cairo_destroy(co_it->local_context);
            cairo_surface_destroy(co_it->control_canvas);

            co_it->control_canvas = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
            co_it->local_context  = cairo_create(co_it->control_canvas);
        }
    });

    if (name == "") name = fmt::format("{:16x}", (uintptr_t)c.get());

    // Avoid adding duplicates
    if (std::find_if(this->children.begin(), this->children.end(), [&](ControlData& c) {
            return (c.name == name);
        }) != this->children.end()) {

        return;
    }

    auto [context, canvas] = this->parent->createChildContext(c.get());
    c->updatePosition(x, y);
    this->children.emplace_back(
        x, y, ControlPositioning::Pixel, context, canvas, std::move(c), name);
}

void ContainerComponent::add(
    float x, float y, ControlPositioning cpos, std::unique_ptr<Control> c, std::string name = "")
{
    c->setResizeCallback([&](Control* co, size_t w, size_t h) {
        auto co_it = std::find_if(
            this->children.begin(), this->children.end(),
            [&](ControlData& cd) { return cd.control->getID() == co->getID(); });

        if (co_it != this->children.end()) {
            cairo_destroy(co_it->local_context);
            cairo_surface_destroy(co_it->control_canvas);

            co_it->control_canvas = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
            co_it->local_context  = cairo_create(co_it->control_canvas);
        }
    });

    if (name == "") name = fmt::format("{:16x}", (uintptr_t)c.get());

    // Avoid adding duplicates
    if (std::find_if(this->children.begin(), this->children.end(), [&](ControlData& c) {
            return (c.name == name);
        }) != this->children.end()) {

        return;
    }

    auto [context, canvas] = this->parent->createChildContext(c.get());
    if (x > 1.1 || y > 1.1 || cpos == ControlPositioning::Pixel) {
        c->updatePosition(x, y);
        this->children.emplace_back(
            (int)x, (int)y, ControlPositioning::Pixel, context, canvas, std::move(c), name);
    } else {
        this->children.emplace_back(x, y, cpos, context, canvas, std::move(c), name);
    }
}

void ContainerComponent::remove(unsigned long long control_id)
{
    auto cd = std::remove_if(this->children.begin(), this->children.end(), [&](ControlData& cd) {
        return (cd.control->getID() == control_id);
    });

    this->children.erase(cd, this->children.end());

    // TODO: see if the control_id specified is the child of our
    //       children.
}

/**
 * Sort the controls by their z-index values
 */
void ContainerComponent::sortZIndex()
{
    std::sort(this->children.begin(), this->children.end(), [](ControlData& a, ControlData& b) {
        return a.control->z_index < b.control->z_index;
    });
}

/**
 * Update the absolute (aka the pixel) positions of a control, so we can keep
 * track of them for box testing, for example
 *
 * When you discover the absolute position based on a relative coordinate
 * (like ControlPositioning::CenterX, or the fractional relative number),
 * you call this function to update it
 */
void ContainerComponent::updateAbsoluteCoord(unsigned long long control_id, int absx, int absy)
{
    auto cd = std::find_if(
        std::begin(this->children), std::end(this->children),
        [&](const ControlData& cd) { return cd.control && (cd.control->getID() == control_id); });

    if (cd != this->children.end()) {
        cd->x = absx;
        cd->y = absy;
        cd->control->updatePosition(absx, absy);
    }
}

/**
 * Get the control that is at the specified pixel coordinate
 */
std::optional<Control*> ContainerComponent::getControlAtPoint(int x, int y)
{
    // Sort the controls by the z-index.
    // Since, now, the lower z-index control will come first, we need to
    // use the std::rbegin, that will get the iterator from the end,
    // and not from the start.
    this->sortZIndex();

    // Use those reference wrapper so we can compile this.
    //
    // The use of that unique pointer in the control data structure did not let the code
    // compile, because unique pointers cannot be copied, which is what the accumulate function
    // probably did. But this reduce loop receives references to the control data, and the ref
    // wrapper can keep them. Sinde we do not change the data, we are good.

    auto cd = std::accumulate(
        std::rbegin(this->children), std::rend(this->children),
        std::optional<std::reference_wrapper<ControlData>>(),
        [&](std::optional<std::reference_wrapper<ControlData>> c,
            ControlData& val) -> std::optional<std::reference_wrapper<ControlData>> {
            if (c) {
                // TODO: check z-indices when they are implemented.
                return c;
            }

            auto ctrlw = cairo_image_surface_get_width(val.control_canvas);
            auto ctrlh = cairo_image_surface_get_height(val.control_canvas);

            if (x > val.x && x < val.x + ctrlw && y > val.y && y < val.y + ctrlh) {
                return std::optional<std::reference_wrapper<ControlData>>(std::ref(val));
            } else {
                return std::nullopt;
            }
        });

    if (cd) {
        return std::optional<Control*>(cd->get().control.get());
    }

    return std::nullopt;
}

/**
 * Gets the control you gave the name `name`
 *
 * Useful for things that require you to change the control in a different thread,
 * or even from a different function it was created.
 */
Control* ContainerComponent::get(std::string name)
{
    auto obj = std::find_if(
        this->children.begin(), this->children.end(),
        [&](const ControlData& cd) { return cd.name == name; });

    if (obj != this->children.end()) {
        return obj->control.get();
    }

    return nullptr;
}
