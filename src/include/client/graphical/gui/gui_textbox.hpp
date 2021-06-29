#pragma once

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <client/graphical/gui/control.hpp>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include <locale>
#include <codecvt>
#include <cwchar>
#include <iomanip>

namespace familyline::graphics::gui
{
/**
 * The label GUI control
 */
class Textbox : public Control
{
private:
    unsigned width_, height_;

    /// The individual code points of our text
    ///
    /// We could use a normal std::string, but then we would not be able
    /// to edit the text, because we have multibyte text in utf-8, and the
    /// array index would not always correspond to the character index
    ///
    /// u32string uses UTF-32, with 4 bytes for each codepoint, sufficient
    /// to be able to store the whole unicode in its array elements, and
    /// array index and character index would match (if we normalize the
    /// string, which we will do)
    std::u32string text_;

    /// The cursor position, an character index that represents where
    /// the cursor is.
    int cursorpos_ = 0;
    
    PangoLayout* layout_before_ = nullptr;
    PangoLayout* layout_after_ = nullptr;
    std::mutex text_mtx_;
    
    PangoLayout* getLayout(cairo_t* context, std::string text) const;
    PangoWeight getPangoWeightFromAppearance(FontWeight fw) const;
    
    cairo_t* last_context_ = nullptr;

    std::string convertUTF32ToUTF8(std::u32string v) const;
    std::u32string convertUTF8ToUTF32(std::string v) const;    
public:
    Textbox(unsigned width, unsigned height, std::string text)
        : width_(width), height_(height)
    {
        this->appearance_.fontFace = "Arial";
        this->appearance_.fontSize = 14;
        this->appearance_.background = {1, 1, 1, 0.9};
        this->appearance_.foreground = {0, 0, 0, 0.95};

        this->setText(text);
    }

    virtual bool update(cairo_t* context, cairo_surface_t* canvas);

    virtual std::tuple<int, int> getNeededSize(cairo_t* parent_context) const;

    void setText(std::string v);
    std::string getText() const;
    
    virtual void receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq);

    virtual void onFocusEnter();
    virtual void onFocusLost();

    int maxChars = 255;
    
    virtual ~Textbox() {
        if (layout_before_)
            g_object_unref(layout_before_);
        
        if (layout_after_)
            g_object_unref(layout_after_);
    }
};

}  // namespace familyline::graphics::gui
