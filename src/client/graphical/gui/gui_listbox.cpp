
#include <algorithm>
#include <client/graphical/gui/gui_listbox.hpp>

using namespace familyline::graphics::gui;
using namespace familyline::input;


bool Listbox::update(cairo_t* context, cairo_surface_t* canvas)
{
    auto [fr, fg, fb, fa] = this->appearance_.foreground;
    auto [br, bg, bb, ba] = this->appearance_.background;

    cairo_set_source_rgba(context, br, bg, bb, ba);
    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_paint(context);

    int i = 0;
    int maxitems = height_ / item_height_;

    list_mtx_.lock();
    for (auto& it: items_) {
        if (current_start_index_ > i) {
            i++;
            continue;            
        }

        if (i > (current_start_index_+maxitems)) {
            break;
        }
                    
        auto y = item_height_*i;

        it.second.control->update(it.second.context, it.second.canvas);

        cairo_set_operator(context, CAIRO_OPERATOR_OVER);
        cairo_set_source_surface(context, it.second.canvas, 0+margin_, y+margin_);
        cairo_paint(context);

        if (it.second.selected) {
            cairo_set_source_rgba(context, 1, 1, 1, 0.2);
            cairo_rectangle(context, 0, y+margin_, width_, item_height_);
            cairo_fill(context);
        }
        
        i++;
    }
    list_mtx_.unlock();
    

    // draw a border
    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_set_line_width(context, margin_);
    cairo_set_source_rgba(context, 0, 0, 0, 1);
    cairo_rectangle(context, 0, 0, width_, height_);
    cairo_stroke(context);

    return true;
}

std::tuple<int, int> Listbox::getNeededSize(cairo_t* parent_context) const
{
    return std::tie(width_, height_);
}

void Listbox::selectItem(std::string code)
{
    std::lock_guard<std::mutex> guard(list_mtx_);

    if (items_.contains(code)) {
        if (selected_item_ != "")
            items_[selected_item_].selected = false;
        
        items_[code].selected = true;
        onSelectItem(code);
        selected_item_ = code;
    }
}


void Listbox::addItem(std::string code, std::unique_ptr<Control> control)
{
    if (code == "") {
        return;
    }
    
    int w=0, h=0;
    cairo_surface_t* canvas = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width_, item_height_);
    cairo_t* context = cairo_create(canvas);

    std::lock_guard<std::mutex> guard(list_mtx_);

    std::tie(w, h) = control->getNeededSize(context);

    items_.emplace(std::make_pair(code, ListboxItem{std::move(control), context, canvas, false}));
}

void Listbox::removeItem(std::string code)
{
    std::lock_guard<std::mutex> guard(list_mtx_);
    cairo_surface_destroy(items_[code].canvas);
    cairo_destroy(items_[code].context);
    items_.erase(code);
}


void Listbox::receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq)
{
    if (std::holds_alternative<ClickAction>(ev.type)) {
        auto ca  = std::get<ClickAction>(ev.type);
        if (ca.isPressed && ca.buttonCode == 1) {
            int rely = ca.screenY -  y_;

            int visibleidx = (rely - margin_) / item_height_;
            int realidx = current_start_index_ + visibleidx;

            if (visibleidx < 0) {
                return;
            }

            
            list_mtx_.lock();

            int itidx = 0;
            auto val = std::find_if(items_.begin(), items_.end(), [&](auto& item) {
                if (itidx == realidx) {
                    return true;
                }
                
                itidx++;
                return false;
            });
            list_mtx_.unlock();
            
            if (val != items_.end()) {
                this->selectItem(val->first);                
            }
        }
    }
}
