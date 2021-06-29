#pragma once

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <client/graphical/gui/control.hpp>
#include <client/graphical/gui/gui_label.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace familyline::graphics::gui
{
/**
 * The listbox GUI control
 */

struct ListboxItem {
    std::unique_ptr<Control> control;
    cairo_t* context;
    cairo_surface_t* canvas;
    bool selected;
};

class Listbox : public Control
{
private:
    unsigned width_, height_;

    // Item height, in pixels
    unsigned item_height_ = 24;

    std::map<std::string, ListboxItem> items_;

    unsigned label_width_, label_height_;
    cairo_t* l_context_        = nullptr;
    cairo_surface_t* l_canvas_ = nullptr;

    /// The first item in the visible part of the listbox is at index...?
    int current_start_index_ = 0;

    // Border margin, in pixels
    int margin_ = 6;
    
    std::string selected_item_ = "";
    std::mutex list_mtx_;
    
public:
    Listbox(unsigned width, unsigned height)
        : width_(width), height_(height)
    {
        this->appearance_.background = {1.0, 1.0, 1.0, 0.4};
    }

    std::function<void(std::string)> onSelectItem = [](std::string v){};
    
    virtual bool update(cairo_t* context, cairo_surface_t* canvas);

    virtual std::tuple<int, int> getNeededSize(cairo_t* parent_context) const;

    virtual void receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq);

    void addItem(std::string code, std::unique_ptr<Control> control);
    void removeItem(std::string code);

    void selectItem(std::string code);
    std::string getSelectedItem() const { return selected_item_; }    
    
    virtual ~Listbox() {}
};

}  // namespace familyline::graphics::gui
