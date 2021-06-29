#pragma once

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <client/graphical/gui/control.hpp>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace familyline::graphics::gui
{
/**
 * The label GUI control
 */
class Label : public Control
{
private:
    unsigned width_, height_;
    std::string text_;
    PangoLayout* layout_ = nullptr;
    std::mutex text_mtx_;
    
    PangoLayout* getLayout(cairo_t* context) const;
    PangoWeight getPangoWeightFromAppearance(FontWeight fw) const;

    cairo_t* last_context_ = nullptr;
    
public:
    Label(unsigned width, unsigned height, std::string text)
        : width_(width), height_(height), text_(text)
    {
        this->appearance_.fontFace = "Arial";
        this->appearance_.fontSize = 14;
    }

    virtual bool update(cairo_t* context, cairo_surface_t* canvas);

    virtual std::tuple<int, int> getNeededSize(cairo_t* parent_context) const;

    void setText(std::string v);

    virtual void receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq) {}

    virtual ~Label() {
        if (layout_)
            g_object_unref(layout_);
    }
};

}  // namespace familyline::graphics::gui
