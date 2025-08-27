/*
    src/layout.cpp -- A collection of useful layout managers

    The grid layout was contributed by Christian Schueller.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/layout.h>
#include <nanogui/widget.h>
#include <nanogui/window.h>
#include <nanogui/theme.h>
#include <nanogui/label.h>
#include <numeric>

NAMESPACE_BEGIN(nanogui)

BoxLayout::BoxLayout(Orientation orientation, Alignment alignment,
    int margin, int spacing)
    : m_orientation(orientation), m_alignment(alignment), m_margin(margin),
    m_spacing(spacing) {
}

Vector2i BoxLayout::preferred_size(NVGcontext* ctx, const Widget* widget) const {
    Vector2i size(2 * m_margin);

    int y_offset = 0;
    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty()) {
        if (m_orientation == Orientation::Vertical)
            size[1] += widget->theme()->m_window_header_height - m_margin / 2;
        else
            y_offset = widget->theme()->m_window_header_height;
    }

    bool first = true;
    int axis1 = (int)m_orientation, axis2 = ((int)m_orientation + 1) % 2;
    for (auto w : widget->children()) {
        if (!w->visible())
            continue;
        if (first)
            first = false;
        else
            size[axis1] += m_spacing;

        Vector2i ps = w->preferred_size(ctx), fs = w->fixed_size();
        Vector2i target_size(
            fs[0] ? fs[0] : ps[0],
            fs[1] ? fs[1] : ps[1]
        );

        size[axis1] += target_size[axis1];
        size[axis2] = std::max(size[axis2], target_size[axis2] + 2 * m_margin);
        first = false;
    }
    // fixed size is the preferred size if it exists.
    Vector2i to_return(size + Vector2i(0, y_offset));
    if (widget->fixed_size().x() != 0)to_return.x() = widget->fixed_size().x();
    if (widget->fixed_size().y() != 0)to_return.y() = widget->fixed_size().y();

    return to_return;
}

void BoxLayout::perform_layout(NVGcontext* ctx, Widget* widget) const {
    Vector2i fs_w = widget->fixed_size();
    Vector2i container_size(
        fs_w[0] ? fs_w[0] : widget->width(),
        fs_w[1] ? fs_w[1] : widget->height()
    );

    int axis1 = (int)m_orientation, axis2 = ((int)m_orientation + 1) % 2;
    int position = m_margin;
    int y_offset = 0;

    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty()) {
        if (m_orientation == Orientation::Vertical) {
            position += widget->theme()->m_window_header_height - m_margin / 2;
        }
        else {
            y_offset = widget->theme()->m_window_header_height;
            container_size[1] -= y_offset;
        }
    }

    bool first = true;
    for (auto w : widget->children()) {
        if (!w->visible())
            continue;
        if (first)
            first = false;
        else
            position += m_spacing;

        Vector2i ps = w->preferred_size(ctx), fs = w->fixed_size();
        Vector2i target_size(
            fs[0] ? fs[0] : ps[0],
            fs[1] ? fs[1] : ps[1]
        );
        Vector2i pos(0, y_offset);

        pos[axis1] = position;

        switch (m_alignment) {
            case Alignment::Minimum:
                pos[axis2] += m_margin;
                break;
            case Alignment::Middle:
                pos[axis2] += (container_size[axis2] - target_size[axis2]) / 2;
                break;
            case Alignment::Maximum:
                pos[axis2] += container_size[axis2] - target_size[axis2] - m_margin * 2;
                break;
            case Alignment::Fill:
                pos[axis2] += m_margin;
                target_size[axis2] = fs[axis2] ? fs[axis2] : (container_size[axis2] - m_margin * 2);
                break;
        }

        w->set_position(pos);
        w->set_size(target_size);
        w->perform_layout(ctx);
        position += target_size[axis1];
    }
}

Vector2i GroupLayout::preferred_size(NVGcontext* ctx, const Widget* widget) const {
    int height = m_margin, width = 2 * m_margin;

    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        height += widget->theme()->m_window_header_height - m_margin / 2;

    bool first = true, indent = false;
    for (auto c : widget->children()) {
        if (!c->visible())
            continue;
        const Label* label = dynamic_cast<const Label*>(c);
        if (!first)
            height += (label == nullptr) ? m_spacing : m_group_spacing;
        first = false;

        Vector2i ps = c->preferred_size(ctx), fs = c->fixed_size();
        Vector2i target_size(
            fs[0] ? fs[0] : ps[0],
            fs[1] ? fs[1] : ps[1]
        );

        bool indent_cur = indent && label == nullptr;
        height += target_size.y();
        width = std::max(width, target_size.x() + 2 * m_margin + (indent_cur ? m_group_indent : 0));

        if (label)
            indent = !label->caption().empty();
    }
    height += m_margin;

    // fixed size is the preferred size if it exists.
    if (widget->fixed_size().x() != 0)width = widget->fixed_size().x();
    if (widget->fixed_size().y() != 0)height = widget->fixed_size().y();
    return Vector2i(width, height);
}

void GroupLayout::perform_layout(NVGcontext* ctx, Widget* widget) const {
    int height = m_margin;
    int available_width = (widget->fixed_width() ? widget->fixed_width() : widget->width()) - 2 * m_margin;
    //int available_height = (widget->fixed_height() ? widget->fixed_height() : widget->height()) - 2 * m_margin;

    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        height += widget->theme()->m_window_header_height - m_margin / 2;

    bool first = true, indent = false;
    for (auto c : widget->children()) {
        if (!c->visible())
            continue;
        const Label* label = dynamic_cast<const Label*>(c);
        if (!first)
            height += (label == nullptr) ? m_spacing : m_group_spacing;
        first = false;

        bool indent_cur = indent && label == nullptr;
        //Vector2i ps = Vector2i(m_expand_horizontally ? available_width - (indent_cur ? m_group_indent : 0) : c->preferred_size(ctx).x(), m_expand_vertically ? available_height : c->preferred_size(ctx).y());
        Vector2i ps = Vector2i(available_width - (indent_cur ? m_group_indent : 0), c->preferred_size(ctx).y());
        Vector2i fs = c->fixed_size();

        Vector2i target_size(
            fs[0] ? fs[0] : ps[0],
            fs[1] ? fs[1] : ps[1]
        );

        c->set_position(Vector2i(m_margin + (indent_cur ? m_group_indent : 0), height));
        c->set_size(target_size);
        c->perform_layout(ctx);

        height += target_size.y();

        if (label)
            indent = !label->caption().empty();
    }
}

Vector2i GridLayout::preferred_size(NVGcontext* ctx,
    const Widget* widget) const {
    /* Compute minimum row / column sizes */
    std::vector<int> grid[2];
    compute_layout(ctx, widget, grid);

    Vector2i size(
        2 * m_margin + std::accumulate(grid[0].begin(), grid[0].end(), 0)
        + std::max((int)grid[0].size() - 1, 0) * m_spacing[0],
        2 * m_margin + std::accumulate(grid[1].begin(), grid[1].end(), 0)
        + std::max((int)grid[1].size() - 1, 0) * m_spacing[1]
    );

    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        size[1] += widget->theme()->m_window_header_height - m_margin / 2;

    // fixed size is the preferred size if it exists.
    Vector2i to_return(size);
    if (widget->fixed_size().x() != 0)to_return.x() = widget->fixed_size().x();
    if (widget->fixed_size().y() != 0)to_return.y() = widget->fixed_size().y();

    return to_return;
}

void GridLayout::compute_layout(NVGcontext* ctx, const Widget* widget, std::vector<int>* grid) const {
    int axis1 = (int)m_orientation, axis2 = (axis1 + 1) % 2;
    size_t num_children = widget->children().size(), visible_children = 0;
    for (auto w : widget->children())
        visible_children += w->visible() ? 1 : 0;

    Vector2i dim;
    dim[axis1] = m_resolution;
    dim[axis2] = (int)((visible_children + m_resolution - 1) / m_resolution);

    grid[axis1].clear(); grid[axis1].resize(dim[axis1], 0);
    grid[axis2].clear(); grid[axis2].resize(dim[axis2], 0);

    size_t child = 0;
    for (int i2 = 0; i2 < dim[axis2]; i2++) {
        for (int i1 = 0; i1 < dim[axis1]; i1++) {
            Widget* w = nullptr;
            do {
                if (child >= num_children)
                    return;
                w = widget->children()[child++];
            } while (!w->visible());

            Vector2i ps = w->preferred_size(ctx);
            Vector2i fs = w->fixed_size();
            Vector2i target_size(
                fs[0] ? fs[0] : ps[0],
                fs[1] ? fs[1] : ps[1]
            );

            grid[axis1][i1] = std::max(grid[axis1][i1], target_size[axis1]);
            grid[axis2][i2] = std::max(grid[axis2][i2], target_size[axis2]);
        }
    }
}

void GridLayout::perform_layout(NVGcontext* ctx, Widget* widget) const {
    Vector2i fs_w = widget->fixed_size();
    Vector2i container_size(
        fs_w[0] ? fs_w[0] : widget->width(),
        fs_w[1] ? fs_w[1] : widget->height()
    );

    /* Compute minimum row / column sizes */
    std::vector<int> grid[2];
    compute_layout(ctx, widget, grid);
    int dim[2] = { (int)grid[0].size(), (int)grid[1].size() };

    Vector2i extra(0);
    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        extra[1] += widget->theme()->m_window_header_height - m_margin / 2;

    /* Strech to size provided by \c widget */
    for (int i = 0; i < 2; i++) {
        int grid_size = 2 * m_margin + extra[i];
        for (int s : grid[i]) {
            grid_size += s;
            if (i + 1 < dim[i])
                grid_size += m_spacing[i];
        }

        if (grid_size < container_size[i]) {
            /* Re-distribute remaining space evenly */
            int gap = container_size[i] - grid_size;
            int g = gap / dim[i];
            int rest = gap - g * dim[i];
            for (int j = 0; j < dim[i]; ++j)
                grid[i][j] += g;
            for (int j = 0; rest > 0 && j < dim[i]; --rest, ++j)
                grid[i][j] += 1;
        }
    }

    int axis1 = (int)m_orientation, axis2 = (axis1 + 1) % 2;
    Vector2i start = m_margin + extra;

    size_t num_children = widget->children().size();
    size_t child = 0;

    Vector2i pos = start;
    for (int i2 = 0; i2 < dim[axis2]; i2++) {
        pos[axis1] = start[axis1];
        for (int i1 = 0; i1 < dim[axis1]; i1++) {
            Widget* w = nullptr;
            do {
                if (child >= num_children)
                    return;
                w = widget->children()[child++];
            } while (!w->visible());

            Vector2i ps = w->preferred_size(ctx);
            Vector2i fs = w->fixed_size();
            Vector2i target_size(
                fs[0] ? fs[0] : ps[0],
                fs[1] ? fs[1] : ps[1]
            );

            Vector2i item_pos(pos);
            for (int j = 0; j < 2; j++) {
                int axis = (axis1 + j) % 2;
                int item = j == 0 ? i1 : i2;
                Alignment align = alignment(axis, item);

                switch (align) {
                    case Alignment::Minimum:
                        break;
                    case Alignment::Middle:
                        item_pos[axis] += (grid[axis][item] - target_size[axis]) / 2;
                        break;
                    case Alignment::Maximum:
                        item_pos[axis] += grid[axis][item] - target_size[axis];
                        break;
                    case Alignment::Fill:
                        target_size[axis] = fs[axis] ? fs[axis] : grid[axis][item];
                        break;
                }
            }
            w->set_position(item_pos);
            w->set_size(target_size);
            w->perform_layout(ctx);
            pos[axis1] += grid[axis1][i1] + m_spacing[axis1];
        }
        pos[axis2] += grid[axis2][i2] + m_spacing[axis2];
    }
}

AdvancedGridLayout::AdvancedGridLayout(const std::vector<int>& cols, const std::vector<int>& rows, int margin)
    : m_cols(cols), m_rows(rows), m_margin(margin) {
    m_col_stretch.resize(m_cols.size(), 0);
    m_row_stretch.resize(m_rows.size(), 0);
}

Vector2i AdvancedGridLayout::preferred_size(NVGcontext* ctx, const Widget* widget) const {
    /* Compute minimum row / column sizes */
    std::vector<int> grid[2];
    compute_layout(ctx, widget, grid);

    Vector2i size(
        std::accumulate(grid[0].begin(), grid[0].end(), 0),
        std::accumulate(grid[1].begin(), grid[1].end(), 0));

    Vector2i extra(2 * m_margin);
    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        extra[1] += widget->theme()->m_window_header_height - m_margin / 2;

    // fixed size is the preferred size if it exists.
    Vector2i to_return(size + extra);
    if (widget->fixed_size().x() != 0)to_return.x() = widget->fixed_size().x();
    if (widget->fixed_size().y() != 0)to_return.y() = widget->fixed_size().y();


    return to_return;
}

void AdvancedGridLayout::perform_layout(NVGcontext* ctx, Widget* widget) const {
    std::vector<int> grid[2];
    compute_layout(ctx, widget, grid);

    grid[0].insert(grid[0].begin(), m_margin);
    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        grid[1].insert(grid[1].begin(), widget->theme()->m_window_header_height + m_margin / 2);
    else
        grid[1].insert(grid[1].begin(), m_margin);

    for (int axis = 0; axis < 2; ++axis) {
        for (size_t i = 1; i < grid[axis].size(); ++i)
            grid[axis][i] += grid[axis][i - 1];

        for (Widget* w : widget->children()) {
            if (!w->visible() || dynamic_cast<const Window*>(w) != nullptr)
                continue;
            Anchor anchor = this->anchor(w);

            int item_pos = grid[axis][anchor.pos[axis]];
            int cell_size = grid[axis][anchor.pos[axis] + anchor.size[axis]] - item_pos;
            int ps = w->preferred_size(ctx)[axis], fs = w->fixed_size()[axis];
            int target_size = fs ? fs : ps;

            switch (anchor.align[axis]) {
                case Alignment::Minimum:
                    break;
                case Alignment::Middle:
                    item_pos += (cell_size - target_size) / 2;
                    break;
                case Alignment::Maximum:
                    item_pos += cell_size - target_size;
                    break;
                case Alignment::Fill:
                    target_size = fs ? fs : cell_size;
                    break;
            }

            Vector2i pos = w->position(), size = w->size();
            pos[axis] = item_pos;
            size[axis] = target_size;
            w->set_position(pos);
            w->set_size(size);
            w->perform_layout(ctx);
        }
    }
}


void AdvancedGridLayout::compute_layout(NVGcontext* ctx, const Widget* widget,
    std::vector<int>* _grid) const {
    Vector2i fs_w = widget->fixed_size();
    Vector2i container_size(
        fs_w[0] ? fs_w[0] : widget->width(),
        fs_w[1] ? fs_w[1] : widget->height()
    );

    Vector2i extra(2 * m_margin);
    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        extra[1] += widget->theme()->m_window_header_height - m_margin / 2;

    container_size -= extra;

    for (int axis = 0; axis < 2; ++axis) {
        std::vector<int>& grid = _grid[axis];
        const std::vector<int>& sizes = axis == 0 ? m_cols : m_rows;
        const std::vector<float>& stretch = axis == 0 ? m_col_stretch : m_row_stretch;
        grid = sizes;

        for (int phase = 0; phase < 2; ++phase) {
            for (auto pair : m_anchor) {
                const Widget* w = pair.first;
                if (!w->visible() || dynamic_cast<const Window*>(w) != nullptr)
                    continue;
                const Anchor& anchor = pair.second;
                if ((anchor.size[axis] == 1) != (phase == 0))
                    continue;

                int ps, fs = w->fixed_size()[axis];
                
                // ALIGNMENT-BASED WIDTH CONSTRAINT
                if (!fs && axis == 0) { // No fixed width and we're calculating width
                    Alignment align = anchor.align[axis];
                    
                    // Calculate available space for this widget's span
                    int available_space = container_size[axis];
                    if (anchor.size[axis] > 1) {
                        // For multi-span widgets, allocate proportionally
                        available_space = (container_size[axis] * anchor.size[axis]) / std::max(1, (int)m_cols.size());
                    } else {
                        // For single-span widgets, use average column width
                        available_space = container_size[axis] / std::max(1, (int)m_cols.size());
                    }
                    
                    // Determine constraint based on alignment intent
                    int max_width_constraint;
                    switch (align) {
                        case Alignment::Fill:
                            // Widget will be resized to fill cell - constrain more aggressively
                            max_width_constraint = std::max(50, available_space);
                            break;
                        case Alignment::Minimum:
                        case Alignment::Maximum:
                        case Alignment::Middle:
                            // Widget keeps preferred size - be more generous but still reasonable
                            max_width_constraint = std::max(100, (int)(available_space * 1.2f));
                            break;
                    }
                    
                    // Apply constraint for flexible widgets (like Labels)
                    const Label* label = dynamic_cast<const Label*>(w);
                    if (label) {
                        Widget* non_const_w = const_cast<Widget*>(w);
                        Vector2i original_size = non_const_w->size();
                        
                        // Temporarily set width constraint for preferred size calculation
                        non_const_w->set_size(Vector2i(max_width_constraint, original_size.y()));
                        non_const_w->perform_layout(ctx);
                        ps = w->preferred_size(ctx)[axis];
                        
                        // Restore original size
                        non_const_w->set_size(original_size);
                    } else {
                        ps = w->preferred_size(ctx)[axis];
                        // Still cap non-label widgets reasonably
                        ps = std::min(ps, max_width_constraint);
                    }
                } else {
                    // For height or fixed-width widgets, use normal preferred size
                    ps = w->preferred_size(ctx)[axis];
                }
                
                int target_size = fs ? fs : ps;

                if (anchor.pos[axis] + anchor.size[axis] > (int)grid.size())
                    throw std::runtime_error(
                        "Advanced grid layout: widget is out of bounds: " +
                        (std::string) anchor);

                int current_size = 0;
                float total_stretch = 0;
                for (int i = anchor.pos[axis];
                    i < anchor.pos[axis] + anchor.size[axis]; ++i) {
                    if (sizes[i] == 0 && anchor.size[axis] == 1)
                        grid[i] = std::max(grid[i], target_size);
                    current_size += grid[i];
                    total_stretch += stretch[i];
                }
                if (target_size <= current_size)
                    continue;
                if (total_stretch == 0)
                    throw std::runtime_error(
                        "Advanced grid layout: no space to place widget: " +
                        (std::string) anchor);
                float amt = (target_size - current_size) / total_stretch;
                for (int i = anchor.pos[axis];
                    i < anchor.pos[axis] + anchor.size[axis]; ++i)
                    grid[i] += (int)std::round(amt * stretch[i]);
            }
        }

        // Final stretch distribution - ensure we don't exceed container bounds
        int current_size = std::accumulate(grid.begin(), grid.end(), 0);
        float total_stretch = std::accumulate(stretch.begin(), stretch.end(), 0.0f);
        if (current_size < container_size[axis] && total_stretch > 0) {
            float amt = (container_size[axis] - current_size) / total_stretch;
            for (size_t i = 0; i < grid.size(); ++i)
                grid[i] += (int)std::round(amt * stretch[i]);
        }
    }
}

FlexLayout::FlexLayout(FlexDirection direction, JustifyContent justify_content, 
                       AlignItems align_items, int margin, int gap)
    : m_direction(direction), m_justify_content(justify_content), 
      m_align_items(align_items), m_flex_wrap(FlexWrap::NoWrap),
      m_margin(margin), m_gap(gap) {
}

Vector2i FlexLayout::preferred_size(NVGcontext *ctx, const Widget *widget) const {
    Vector2i size(2 * m_margin);
    
    // Account for window header
    int y_offset = 0;
    const Window *window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty()) {
        if (!is_row_direction())
            size[1] += widget->theme()->m_window_header_height - m_margin / 2;
        else
            y_offset = widget->theme()->m_window_header_height;
    }

    // Collect visible children
    std::vector<Widget*> visible_children;
    for (auto child : widget->children()) {
        if (child->visible())
            visible_children.push_back(child);
    }

    if (visible_children.empty())
        return size + Vector2i(0, y_offset);

    int main_axis_idx = main_axis();
    int cross_axis_idx = cross_axis();
    
    int total_main_size = 0;
    int max_cross_size = 0;
    
    for (size_t i = 0; i < visible_children.size(); ++i) {
        Widget *child = visible_children[i];
        Vector2i child_pref = child->preferred_size(ctx);
        Vector2i child_fixed = child->fixed_size();
        
        FlexItem flex_item = get_flex_item(child);
        
        // Main axis size
        int main_size = child_fixed[main_axis_idx] ? child_fixed[main_axis_idx] : 
                       (flex_item.flex_basis >= 0 ? flex_item.flex_basis : child_pref[main_axis_idx]);
        total_main_size += main_size;
        
        // Cross axis size  
        int cross_size = child_fixed[cross_axis_idx] ? child_fixed[cross_axis_idx] : child_pref[cross_axis_idx];
        max_cross_size = std::max(max_cross_size, cross_size);
        
        // Add gap (except for last item)
        if (i < visible_children.size() - 1)
            total_main_size += m_gap;
    }
    
    size[main_axis_idx] += total_main_size;
    size[cross_axis_idx] += max_cross_size;
    
    Vector2i result = size + Vector2i(0, y_offset);
    
    // Apply fixed size constraints
    if (widget->fixed_size().x() != 0) result.x() = widget->fixed_size().x();
    if (widget->fixed_size().y() != 0) result.y() = widget->fixed_size().y();
    
    return result;
}

void FlexLayout::perform_layout(NVGcontext *ctx, Widget *widget) const {
    Vector2i container_size = widget->size();
    Vector2i fs_w = widget->fixed_size();
    if (fs_w.x()) container_size.x() = fs_w.x();
    if (fs_w.y()) container_size.y() = fs_w.y();

    int y_offset = 0;
    const Window *window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty()) {
        if (!is_row_direction()) {
            container_size.y() -= widget->theme()->m_window_header_height - m_margin / 2;
        } else {
            y_offset = widget->theme()->m_window_header_height;
            container_size.y() -= y_offset;
        }
    }

    // Collect visible children
    std::vector<Widget*> visible_children;
    for (auto child : widget->children()) {
        if (child->visible())
            visible_children.push_back(child);
    }

    if (visible_children.empty())
        return;

    int main_axis_idx = main_axis();
    int cross_axis_idx = cross_axis();
    
    // Available space for flex items (subtract margins)
    int available_main_space = container_size[main_axis_idx] - 2 * m_margin;
    int available_cross_space = container_size[cross_axis_idx] - 2 * m_margin;
    
    // Calculate base sizes and collect flex info
    std::vector<int> base_sizes;
    std::vector<int> final_sizes;
    float total_flex_grow = 0.0f;
    float total_flex_shrink_scaled = 0.0f;
    int total_base_size = 0;
    int total_gaps = std::max(0, (int)visible_children.size() - 1) * m_gap;
    
    for (Widget *child : visible_children) {
        Vector2i child_pref = child->preferred_size(ctx);
        Vector2i child_fixed = child->fixed_size();
        FlexItem flex_item = get_flex_item(child);
        
        int base_size;
        if (child_fixed[main_axis_idx]) {
            base_size = child_fixed[main_axis_idx];
        } else if (flex_item.flex_basis >= 0) {
            base_size = flex_item.flex_basis;
        } else {
            base_size = child_pref[main_axis_idx];
        }
        
        base_sizes.push_back(base_size);
        total_base_size += base_size;
        total_flex_grow += flex_item.flex_grow;
        total_flex_shrink_scaled += flex_item.flex_shrink * base_size;
    }
    
    // Resolve flexible lengths[4]
    int remaining_space = available_main_space - total_base_size - total_gaps;
    
    for (size_t i = 0; i < visible_children.size(); ++i) {
        FlexItem flex_item = get_flex_item(visible_children[i]);
        int final_size = base_sizes[i];
        
        if (remaining_space > 0 && total_flex_grow > 0) {
            // Distribute positive space via flex-grow
            float grow_factor = flex_item.flex_grow / total_flex_grow;
            final_size += (int)(remaining_space * grow_factor);
        } else if (remaining_space < 0 && total_flex_shrink_scaled > 0) {
            // Distribute negative space via flex-shrink  
            float shrink_factor = (flex_item.flex_shrink * base_sizes[i]) / total_flex_shrink_scaled;
            final_size += (int)(remaining_space * shrink_factor);
            final_size = std::max(0, final_size); // Don't allow negative sizes
        }
        
        final_sizes.push_back(final_size);
    }
    
    // Position items along main axis based on justify_content[2][3]
    std::vector<int> positions;
    int current_pos = m_margin;
    
    switch (m_justify_content) {
        case JustifyContent::FlexStart:
            for (size_t i = 0; i < visible_children.size(); ++i) {
                positions.push_back(current_pos);
                current_pos += final_sizes[i] + (i < visible_children.size() - 1 ? m_gap : 0);
            }
            break;
            
        case JustifyContent::FlexEnd: {
            int total_used = std::accumulate(final_sizes.begin(), final_sizes.end(), 0) + total_gaps;
            current_pos = available_main_space + m_margin - total_used;
            for (size_t i = 0; i < visible_children.size(); ++i) {
                positions.push_back(current_pos);
                current_pos += final_sizes[i] + (i < visible_children.size() - 1 ? m_gap : 0);
            }
            break;
        }
        
        case JustifyContent::Center: {
            int total_used = std::accumulate(final_sizes.begin(), final_sizes.end(), 0) + total_gaps;
            current_pos = m_margin + (available_main_space - total_used) / 2;
            for (size_t i = 0; i < visible_children.size(); ++i) {
                positions.push_back(current_pos);
                current_pos += final_sizes[i] + (i < visible_children.size() - 1 ? m_gap : 0);
            }
            break;
        }
        
        case JustifyContent::SpaceBetween: {
            if (visible_children.size() == 1) {
                positions.push_back(m_margin);
            } else {
                int total_item_size = std::accumulate(final_sizes.begin(), final_sizes.end(), 0);
                int space_between = (available_main_space - total_item_size) / ((int)visible_children.size() - 1);
                current_pos = m_margin;
                for (size_t i = 0; i < visible_children.size(); ++i) {
                    positions.push_back(current_pos);
                    current_pos += final_sizes[i] + space_between;
                }
            }
            break;
        }
        
        case JustifyContent::SpaceAround: {
            int total_item_size = std::accumulate(final_sizes.begin(), final_sizes.end(), 0);
            int space_around = (available_main_space - total_item_size) / (2 * (int)visible_children.size());
            current_pos = m_margin + space_around;
            for (size_t i = 0; i < visible_children.size(); ++i) {
                positions.push_back(current_pos);
                current_pos += final_sizes[i] + 2 * space_around;
            }
            break;
        }
        
        case JustifyContent::SpaceEvenly: {
            int total_item_size = std::accumulate(final_sizes.begin(), final_sizes.end(), 0);
            int space_evenly = (available_main_space - total_item_size) / ((int)visible_children.size() + 1);
            current_pos = m_margin + space_evenly;
            for (size_t i = 0; i < visible_children.size(); ++i) {
                positions.push_back(current_pos);
                current_pos += final_sizes[i] + space_evenly;
            }
            break;
        }
    }
    
    // Handle reverse directions
    if (is_reverse_direction()) {
        std::reverse(positions.begin(), positions.end());
        std::reverse(final_sizes.begin(), final_sizes.end());
        // Adjust positions for reverse
        for (size_t i = 0; i < positions.size(); ++i) {
            positions[i] = container_size[main_axis_idx] - positions[i] - final_sizes[i];
        }
    }
    
    // Apply positions and sizes to children
    for (size_t i = 0; i < visible_children.size(); ++i) {
        Widget *child = visible_children[i];
        Vector2i child_pos = child->position();
        Vector2i child_size = child->size();
        FlexItem flex_item = get_flex_item(child);
        
        // Set main axis position and size
        child_pos[main_axis_idx] = positions[i];
        child_size[main_axis_idx] = final_sizes[i];
        
        // Handle cross axis alignment
        Vector2i child_pref = child->preferred_size(ctx);
        Vector2i child_fixed = child->fixed_size();
        
        AlignItems align = (flex_item.align_self != AlignItems::FlexStart) ? flex_item.align_self : m_align_items;
        int cross_size = child_fixed[cross_axis_idx] ? child_fixed[cross_axis_idx] : child_pref[cross_axis_idx];
        
        switch (align) {
            case AlignItems::FlexStart:
                child_pos[cross_axis_idx] = m_margin;
                child_size[cross_axis_idx] = cross_size;
                break;
            case AlignItems::FlexEnd:
                child_pos[cross_axis_idx] = container_size[cross_axis_idx] - cross_size - m_margin;
                child_size[cross_axis_idx] = cross_size;
                break;
            case AlignItems::Center:
                child_pos[cross_axis_idx] = m_margin + (available_cross_space - cross_size) / 2;
                child_size[cross_axis_idx] = cross_size;
                break;
            case AlignItems::Stretch:
                child_pos[cross_axis_idx] = m_margin;
                child_size[cross_axis_idx] = child_fixed[cross_axis_idx] ? child_fixed[cross_axis_idx] : available_cross_space;
                break;
            case AlignItems::Baseline:
                // Simplified baseline alignment (treat as flex-start for now)
                child_pos[cross_axis_idx] = m_margin;
                child_size[cross_axis_idx] = cross_size;
                break;
        }
        
        // Apply y_offset for window headers
        if (y_offset > 0)
            child_pos.y() += y_offset;
            
        child->set_position(child_pos);
        child->set_size(child_size);
        child->perform_layout(ctx);
    }
}

NAMESPACE_END(nanogui)
