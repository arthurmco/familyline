
#include <SDL2/SDL_keycode.h>
#include <client/graphical/gui/gui_textbox.hpp>
#include <client/input/input_service.hpp>
#include <mutex>

#include "pango/pango-font.h"

using namespace familyline::graphics::gui;
using namespace familyline::input;

/// String converter class, converts between utf32 and utf8
auto& cvtr = std::use_facet<std::codecvt<char32_t, char, std::mbstate_t>>(std::locale());

PangoWeight Textbox::getPangoWeightFromAppearance(FontWeight fw) const
{
    switch (fw) {
        case FontWeight::Thin: return PANGO_WEIGHT_THIN; break;
        case FontWeight::Ultralight: return PANGO_WEIGHT_ULTRALIGHT; break;
        case FontWeight::Light: return PANGO_WEIGHT_LIGHT; break;
        case FontWeight::Semilight: return PANGO_WEIGHT_SEMILIGHT; break;
        case FontWeight::Book: return PANGO_WEIGHT_BOOK; break;
        case FontWeight::Normal: return PANGO_WEIGHT_NORMAL; break;
        case FontWeight::Medium: return PANGO_WEIGHT_MEDIUM; break;
        case FontWeight::Semibold: return PANGO_WEIGHT_SEMIBOLD; break;
        case FontWeight::Bold: return PANGO_WEIGHT_BOLD; break;
        case FontWeight::Ultrabold: return PANGO_WEIGHT_ULTRABOLD; break;
        case FontWeight::Heavy: return PANGO_WEIGHT_HEAVY; break;
        case FontWeight::Ultraheavy: return PANGO_WEIGHT_ULTRAHEAVY; break;
    }

    return PANGO_WEIGHT_NORMAL;
}

PangoLayout* Textbox::getLayout(cairo_t* context, std::string text) const
{
    PangoFontDescription* font_description = pango_font_description_new();
    pango_font_description_set_family(font_description, this->appearance_.fontFace.c_str());
    pango_font_description_set_absolute_size(
        font_description, this->appearance_.fontSize * PANGO_SCALE);
    pango_font_description_set_weight(
        font_description, getPangoWeightFromAppearance(appearance_.fontWeight));
    pango_font_description_set_style(
        font_description, appearance_.italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);

    PangoLayout* layout = pango_cairo_create_layout(context);
    pango_layout_set_font_description(layout, font_description);
    pango_layout_set_text(layout, text.c_str(), -1);

    pango_font_description_free(font_description);

    return layout;
}

bool Textbox::update(cairo_t* context, cairo_surface_t* canvas)
{
    auto [fr, fg, fb, fa] = this->appearance_.foreground;
    auto [br, bg, bb, ba] = this->appearance_.background;

    if (layout_before_) {
        pango_layout_set_font_description(layout_before_, nullptr);
        g_object_unref(layout_before_);
    }
    if (layout_after_) {
        pango_layout_set_font_description(layout_after_, nullptr);
        g_object_unref(layout_after_);
    }

    std::lock_guard<std::mutex> guard(text_mtx_);

    layout_before_ =
        this->getLayout(context, this->convertUTF32ToUTF8(this->text_.substr(0, cursorpos_)));
    int textw = 0, texth = 0;

    cairo_set_source_rgba(context, br, bg, bb, ba);
    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_paint(context);

    // draw text before the cursor
    pango_layout_get_size(layout_before_, &textw, &texth);
    texth /= PANGO_SCALE;

    cairo_set_operator(context, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(context, fr, fg, fb, fa);
    cairo_move_to(context, 6, (height_ / 2) - (texth / 2));
    pango_cairo_show_layout(context, layout_before_);

    layout_after_ =
        this->getLayout(context, this->convertUTF32ToUTF8(this->text_.substr(cursorpos_)));

    int beforew = textw / PANGO_SCALE;
    // draw text after the cursor
    pango_layout_get_size(layout_after_, &textw, &texth);
    texth /= PANGO_SCALE;

    cairo_set_source_rgba(context, fr, fg, fb, fa);
    cairo_move_to(context, 6 + beforew, (height_ / 2) - (texth / 2));
    pango_cairo_show_layout(context, layout_after_);

    // draw the cursor
    cairo_set_line_width(context, 1);
    cairo_set_source_rgba(context, 0, 0, 0, 0.75);
    cairo_rectangle(context, 6 + beforew - 1, 0, 2, height_);
    cairo_fill(context);

    // draw a border
    cairo_set_line_width(context, 6);
    cairo_set_source_rgba(context, 0, 0, 0, 1);
    cairo_rectangle(context, 0, 0, width_, height_);
    cairo_stroke(context);

    return true;
}

std::tuple<int, int> Textbox::getNeededSize(cairo_t* parent_context) const
{
    return std::tie(width_, height_);
}

std::string Textbox::convertUTF32ToUTF8(std::u32string v) const
{
    // wstring_convert is deprecated, we need to do this ourselves.
    std::string s8(v.size() * cvtr.max_length(), '\0');

    {
        std::mbstate_t mb{};
        const char32_t* from_next;
        char* to_next;
        cvtr.out(mb, &v[0], &v[v.size()], from_next, &s8[0], &s8[s8.size()], to_next);

        s8.resize(to_next - &s8[0]);
    }

    return s8;
}

std::u32string Textbox::convertUTF8ToUTF32(std::string v) const
{
    std::u32string ns32(v.size() * cvtr.max_length(), '\0');
    {
        std::mbstate_t mb{};
        char32_t* to_next;
        const char* from_next;
        auto ret =
            cvtr.in(mb, &v[0], &v[v.size()], from_next, &ns32[0], &ns32[ns32.size()], to_next);

        ns32.resize(to_next - &ns32[0]);
    }

    return ns32;
}

std::string Textbox::getText() const { return this->convertUTF32ToUTF8(this->text_); }

void Textbox::setText(std::string v)
{
    std::u32string ns32 = this->convertUTF8ToUTF32(v);
    std::lock_guard<std::mutex> guard(text_mtx_);
    if (this->text_ != ns32) {
        if (ns32.size() < cursorpos_) {
            cursorpos_ = ns32.size();
        }
        
        this->text_ = ns32;
    }
}

void Textbox::onFocusEnter() { InputService::getInputManager()->enableTextEvents(); }
void Textbox::onFocusLost() { InputService::getInputManager()->disableTextEvents(); }

void Textbox::receiveEvent(const familyline::input::HumanInputAction& ev, CallbackQueue& cq)
{
    if (std::holds_alternative<KeyAction>(ev.type)) {
        auto ka = std::get<KeyAction>(ev.type);

        if (ka.isPressed) {
            switch (ka.keycode) {
            case SDLK_LEFT: cursorpos_ = std::max(0, cursorpos_ - 1); break;
            case SDLK_RIGHT: cursorpos_ = std::min((int)text_.size(), cursorpos_ + 1); break;
            case SDLK_HOME: cursorpos_ = 0; break;
            case SDLK_END: cursorpos_ = text_.size(); break;
            case SDLK_BACKSPACE:
                if (cursorpos_ > 0) {
                    text_.erase(cursorpos_-1, 1);
                    cursorpos_--;
                }
                break;
            case SDLK_DELETE:
                if (text_.size() > 0) {
                    text_.erase(cursorpos_, 1);
                }
            case SDLK_v:
                if ((ka.modifiers & KMOD_CTRL) > 0) {
                    auto clipboard = InputService::getInputManager()->getClipboardText();
                    std::erase_if(clipboard, [](char v) { return v == '\n' || v == '\r'; });
                    auto text32 = this->convertUTF8ToUTF32(clipboard);
                    text_.insert(cursorpos_, text32);
                    cursorpos_ += text32.size();
                }
                
                break;
            }
        }
    }

    if (std::holds_alternative<TextInput>(ev.type)) {
        auto ti = std::get<TextInput>(ev.type);

        auto text32 = this->convertUTF8ToUTF32(ti.text);
        if (cursorpos_ + text32.size() > maxChars) {
            return;
        }
        
        text_.insert(cursorpos_, text32);
        cursorpos_ += text32.size();        
    }

}
