/*
    nanogui/label.h -- Text label with an arbitrary font, color, and size

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
/** \file */

#pragma once

#include <nanogui/widget.h>

NAMESPACE_BEGIN(nanogui)

/**
 * \class Label label.h nanogui/label.h
 *
 * \brief Text label widget.
 *
 * The font and color can be customized. When \ref Widget::set_fixed_width()
 * is used, the text is wrapped when it surpasses the specified width.
 */
class NANOGUI_EXPORT Label : public Widget {
public:
    /// Line breaking modes similar to NSLabel
    enum class LineBreakMode {
        BreakByWordWrapping,        ///< Wrap at word boundaries
        LineBreakByCharWrapping,    ///< Wrap at any character
        LineBreakByClipping,        ///< Clip text that doesn't fit
        LineBreakByTruncatingHead,  ///< Truncate from beginning with "..."
        LineBreakByTruncatingTail,  ///< Truncate from end with "..."
        LineBreakByTruncatingMiddle ///< Truncate from middle with "..."
    };

    Label(Widget *parent, const std::string &caption,
          const std::string &font = "sans", int font_size = -1);

    /// Get the label's text caption
    const std::string &caption() const { return m_caption; }
    /// Set the label's text caption
    void set_caption(const std::string &caption) { m_caption = caption; }

    /// Set the currently active font (2 are available by default: 'sans' and 'sans-bold')
    void set_font(const std::string &font) { m_font = font; }
    /// Get the currently active font
    const std::string &font() const { return m_font; }

    /// Get the label color
    Color color() const { return m_color; }
    /// Set the label color
    void set_color(const Color& color) { m_color = color; }

    /// Get the line break mode
    LineBreakMode line_break_mode() const { return m_line_break_mode; }
    /// Set the line break mode
    void set_line_break_mode(LineBreakMode mode) { m_line_break_mode = mode; }

    /// Set the \ref Theme used to draw this widget
    virtual void set_theme(Theme *theme) override;

    /// Compute the size needed to fully display the label
    virtual Vector2i preferred_size(NVGcontext *ctx) const override;

    /// Draw the label
    virtual void draw(NVGcontext *ctx) override;

protected:
    /// Process text according to the line break mode
    std::string process_text_for_mode(NVGcontext *ctx, const std::string &text, float available_width) const;
    
    /// Wrap text 
    std::string wrap_by_character(NVGcontext *ctx, const std::string &text, float available_width) const;

    /// Truncate text with ellipsis at the head
    std::string truncate_head(NVGcontext *ctx, const std::string &text, float available_width) const;
    
    /// Truncate text with ellipsis at the tail
    std::string truncate_tail(NVGcontext *ctx, const std::string &text, float available_width) const;
    
    /// Truncate text with ellipsis in the middle
    std::string truncate_middle(NVGcontext *ctx, const std::string &text, float available_width) const;

	/// Clip text to fit within available width without ellipsis
    std::string clip_text(NVGcontext *ctx, const std::string &text, float available_width) const;

    std::string m_caption;
    std::string m_font;
    mutable std::string m_processed_text;
    Color m_color;
    LineBreakMode m_line_break_mode;
};

NAMESPACE_END(nanogui)

