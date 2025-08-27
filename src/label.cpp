/*
arc/label.cpp -- Text label with an arbitrary font, color, and size
	}

text
NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
The widget drawing code is based on the NanoVG demo application
by Mikko Mononen.

All rights reserved. Use of this source code is governed by a
BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/label.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <algorithm>
#include <sstream>

NAMESPACE_BEGIN(nanogui)

Label::Label(Widget *parent, const std::string &caption, const std::string &font, int font_size)
    : Widget(parent), m_caption(caption), m_font(font), m_line_break_mode(LineBreakMode::BreakByWordWrapping) {
    DebugName = m_parent->DebugName + ",Labl";
    if (m_theme) {
        m_font_size = m_theme->m_standard_font_size;
        m_color = m_theme->m_text_color;
    }
    if (font_size >= 0) m_font_size = font_size;
    m_processed_text = ""; 
}

void Label::set_theme(Theme *theme) {
    Widget::set_theme(theme);
    if (m_theme) {
        m_font_size = m_theme->m_standard_font_size;
        m_color = m_theme->m_text_color;
    }
}

Vector2i Label::preferred_size(NVGcontext *ctx) const {
    if (m_caption == "")
        return Vector2i(0);
    
    nvgFontFace(ctx, m_font.c_str());
    nvgFontSize(ctx, font_size());

	m_processed_text = process_text_for_mode(ctx, m_caption, m_fixed_size.x());
    
    if (m_fixed_size.x() > 0) {
        float bounds[4];
        nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        
        switch (m_line_break_mode) {
            case LineBreakMode::BreakByWordWrapping:
                nvgTextBoxBounds(ctx, m_pos.x(), m_pos.y(), m_fixed_size.x(), m_caption.c_str(), nullptr, bounds);
                return Vector2i(m_fixed_size.x(), bounds[3] - bounds[1]);
                
            case LineBreakMode::LineBreakByCharWrapping: {
                // For character wrapping, we need to calculate bounds differently
                nvgTextBounds(ctx, 0, 0, m_processed_text.c_str(), nullptr, bounds);
                return Vector2i(m_fixed_size.x(), bounds[3] - bounds[1]);
            }
            
            case LineBreakMode::LineBreakByClipping:
            case LineBreakMode::LineBreakByTruncatingHead:
            case LineBreakMode::LineBreakByTruncatingTail:
            case LineBreakMode::LineBreakByTruncatingMiddle:
                // Single line height for truncating modes
                return Vector2i(m_fixed_size.x(), font_size());
        }
    } else {
        nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        return Vector2i(
            nvgTextBounds(ctx, 0, 0, m_caption.c_str(), nullptr, nullptr) + 2,
            font_size()
        );
    }
    
    return Vector2i(0);
}

void Label::draw(NVGcontext *ctx) {
    Widget::draw(ctx);
    nvgFontFace(ctx, m_font.c_str());
    nvgFontSize(ctx, font_size());
    nvgFillColor(ctx, m_color);
    
    if (m_fixed_size.x() > 0) {
        switch (m_line_break_mode) {
            case LineBreakMode::BreakByWordWrapping:
                nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
                nvgTextBox(ctx, m_pos.x(), m_pos.y(), m_fixed_size.x(), m_processed_text.c_str(), nullptr);
                break;
                
            case LineBreakMode::LineBreakByCharWrapping:
                nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
                nvgTextBox(ctx, m_pos.x(), m_pos.y(), m_fixed_size.x(), m_processed_text.c_str(), nullptr);
                break;
                
            case LineBreakMode::LineBreakByClipping:
            case LineBreakMode::LineBreakByTruncatingHead:
            case LineBreakMode::LineBreakByTruncatingTail:
            case LineBreakMode::LineBreakByTruncatingMiddle:
                nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                nvgText(ctx, m_pos.x(), m_pos.y() + m_size.y() * 0.5f, m_processed_text.c_str(), nullptr);
                break;
        }
    } else {
        nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(ctx, m_pos.x(), m_pos.y() + m_size.y() * 0.5f, m_caption.c_str(), nullptr);
    }
}

std::string Label::process_text_for_mode(NVGcontext *ctx, const std::string &text, float available_width) const {
    switch (m_line_break_mode) {
        case LineBreakMode::BreakByWordWrapping:
            return text; // nvgTextBox handles this automatically
            
        case LineBreakMode::LineBreakByCharWrapping:
            return wrap_by_character(ctx, text, available_width);
            
        case LineBreakMode::LineBreakByClipping:
            return clip_text(ctx, text, available_width);
            
        case LineBreakMode::LineBreakByTruncatingHead:
            return truncate_head(ctx, text, available_width);
            
        case LineBreakMode::LineBreakByTruncatingTail:
            return truncate_tail(ctx, text, available_width);
            
        case LineBreakMode::LineBreakByTruncatingMiddle:
            return truncate_middle(ctx, text, available_width);
    }
    return text;
}

std::string Label::wrap_by_character(NVGcontext *ctx, const std::string &text, float available_width) const {
    std::string result;
    std::string current_line;
    
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        std::string test_line = current_line + c;
        
        float bounds[4];
        nvgTextBounds(ctx, 0, 0, test_line.c_str(), nullptr, bounds);
        float line_width = bounds[2] - bounds[0];
        
        if (line_width > available_width && !current_line.empty()) {
            result += current_line + '\n';
            current_line = c;
        } else {
            current_line += c;
        }
    }
    
    if (!current_line.empty()) {
        result += current_line;
    }

	//printf("wrap_by_char: [%s] \n", result.c_str() );
    
    return result;
}

std::string Label::truncate_head(NVGcontext *ctx, const std::string &text, float available_width) const {
    const std::string ellipsis = "...";
    
    float bounds[4];
    nvgTextBounds(ctx, 0, 0, text.c_str(), nullptr, bounds);
    float text_width = bounds[2] - bounds[0];
    
    if (text_width <= available_width) {
        return text;
    }
    
    nvgTextBounds(ctx, 0, 0, ellipsis.c_str(), nullptr, bounds);
    float ellipsis_width = bounds[2] - bounds[0];
    float target_width = available_width - ellipsis_width;
    
    if (target_width <= 0) {
        return ellipsis;
    }
    
    // Binary search to find the right substring length
    int left = 0, right = text.length();
    std::string best_fit;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        std::string candidate = text.substr(text.length() - mid);
        
        nvgTextBounds(ctx, 0, 0, candidate.c_str(), nullptr, bounds);
        float candidate_width = bounds[2] - bounds[0];
        
        if (candidate_width <= target_width) {
            best_fit = candidate;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return ellipsis + best_fit;
}

std::string Label::truncate_tail(NVGcontext *ctx, const std::string &text, float available_width) const {
    const std::string ellipsis = "...";
    
    float bounds[4];
    nvgTextBounds(ctx, 0, 0, text.c_str(), nullptr, bounds);
    float text_width = bounds[2] - bounds[0];
    
    if (text_width <= available_width) {
        return text;
    }
    
    nvgTextBounds(ctx, 0, 0, ellipsis.c_str(), nullptr, bounds);
    float ellipsis_width = bounds[2] - bounds[0];
    float target_width = available_width - ellipsis_width;
    
    if (target_width <= 0) {
        return ellipsis;
    }
    
    // Binary search to find the right substring length
    int left = 0, right = text.length();
    std::string best_fit;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        std::string candidate = text.substr(0, mid);
        
        nvgTextBounds(ctx, 0, 0, candidate.c_str(), nullptr, bounds);
        float candidate_width = bounds[2] - bounds[0];
        
        if (candidate_width <= target_width) {
            best_fit = candidate;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return best_fit + ellipsis;
}

std::string Label::truncate_middle(NVGcontext *ctx, const std::string &text, float available_width) const {
    const std::string ellipsis = "...";
    
    float bounds[4];
    nvgTextBounds(ctx, 0, 0, text.c_str(), nullptr, bounds);
    float text_width = bounds[2] - bounds[0];
    
    if (text_width <= available_width) {
        return text;
    }
    
    nvgTextBounds(ctx, 0, 0, ellipsis.c_str(), nullptr, bounds);
    float ellipsis_width = bounds[2] - bounds[0];
    float target_width = available_width - ellipsis_width;
    
    if (target_width <= 0) {
        return ellipsis;
    }
    
    // Binary search to find the right combination of head and tail
    int left = 0, right = text.length() / 2;
    std::string best_result = ellipsis;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int head_len = mid;
        int tail_len = mid;
        
        if (head_len + tail_len >= (int)text.length()) {
            right = mid - 1;
            continue;
        }
        
        std::string head = text.substr(0, head_len);
        std::string tail = text.substr(text.length() - tail_len);
        std::string candidate = head + tail;
        
        nvgTextBounds(ctx, 0, 0, candidate.c_str(), nullptr, bounds);
        float candidate_width = bounds[2] - bounds[0];
        
        if (candidate_width <= target_width) {
            best_result = head + ellipsis + tail;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return best_result;
}

std::string Label::clip_text(NVGcontext *ctx, const std::string &text, float available_width) const {
    float bounds[4];
    nvgTextBounds(ctx, 0, 0, text.c_str(), nullptr, bounds);
    float text_width = bounds[2] - bounds[0];
    
    if (text_width <= available_width) {
        return text;
    }
    
    // Binary search to find the longest substring that fits
    int left = 0, right = text.length();
    std::string best_fit;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        std::string candidate = text.substr(0, mid);
        
        nvgTextBounds(ctx, 0, 0, candidate.c_str(), nullptr, bounds);
        float candidate_width = bounds[2] - bounds[0];
        
        if (candidate_width <= available_width) {
            best_fit = candidate;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return best_fit;
}

NAMESPACE_END(nanogui)

