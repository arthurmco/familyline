
#include <client/graphical/gui/gui_label.hpp>
#include <mutex>

#include "pango/pango-font.h"

using namespace familyline::graphics::gui;

PangoWeight Label::getPangoWeightFromAppearance(FontWeight fw) const
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

PangoLayout* Label::getLayout(cairo_t* context) const
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
    pango_layout_set_markup(layout, this->text_.c_str(), -1);

    pango_font_description_free(font_description);

    return layout;
}

bool Label::update(cairo_t* context, cairo_surface_t* canvas)
{
    auto [fr, fg, fb, fa] = this->appearance_.foreground;
    auto [br, bg, bb, ba] = this->appearance_.background;

    if (layout_) {
        pango_layout_set_font_description(layout_, nullptr);
        g_object_unref(layout_);
    }

    std::lock_guard<std::mutex> guard(text_mtx_);

    layout_ = this->getLayout(context);

    cairo_set_source_rgba(context, br, bg, bb, ba);
    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_paint(context);

    cairo_set_source_rgba(context, fr, fg, fb, fa);
    cairo_move_to(context, 0, 0);
    pango_cairo_show_layout(context, layout_);

    last_context_ = context;

    return true;
}

std::tuple<int, int> Label::getNeededSize(cairo_t* parent_context) const
{
    auto width  = 1;
    auto height = 1;

    if (!parent_context) {
        return std::tie(width, height);
    }

    PangoLayout* layout = this->getLayout(parent_context);

    pango_layout_get_size(layout, &width, &height);
    width /= PANGO_SCALE;
    height /= PANGO_SCALE;

    pango_layout_set_font_description(layout, nullptr);
    g_object_unref(layout);

    return std::tie(width, height);
}

void Label::setText(std::string v)
{
    std::lock_guard<std::mutex> guard(text_mtx_);
    if (this->text_ != v) {
        this->text_ = v;

        if (last_context_) {
            auto width  = 1;
            auto height = 1;

            PangoLayout* layout = this->getLayout(last_context_);
            pango_layout_get_size(layout, &width, &height);
            pango_layout_set_font_description(layout, nullptr);

            last_context_ = nullptr;
            this->resize(width / PANGO_SCALE, height / PANGO_SCALE);
            g_object_unref(layout);
        }
    }
}
