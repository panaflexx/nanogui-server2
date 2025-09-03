#include <nanogui/layout.h>
#include <nanogui/widget.h>
#include <nanogui/window.h>
#include <nanogui/theme.h>
#include <nanogui/label.h>
#include <nanogui/scrollpanel.h>
#include <numeric>
#include <cmath>

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
    int header_adjust = 0;
    if (window && !window->title().empty()) {
        header_adjust = window->theme()->m_window_header_height - m_margin / 2;
        if (m_orientation == Orientation::Vertical)
            size[1] += header_adjust;
        else
            y_offset = header_adjust;
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

        Vector2i ps = w->preferred_size(ctx);
        Vector2i min_s = w->min_size();
        Vector2i max_s = w->max_size();

        Vector2i clamped_pref = ps;
        clamped_pref = max(clamped_pref, min_s);
        // Use parent widget size as default, with fallback to avoid recursion
        Vector2i parent_size = widget->size();
        Vector2i default_size = widget->parent() && widget->parent()->size() != Vector2i(0, 0)
            ? widget->parent()->size() : Vector2i(1000, 1000);
        if (parent_size == Vector2i(0, 0)) parent_size = default_size;
        if (max_s.x() == 0) clamped_pref.x() = std::min(clamped_pref.x(), parent_size.x() - 2 * m_margin);
        if (max_s.y() == 0) clamped_pref.y() = std::min(clamped_pref.y(), parent_size.y() - 2 * m_margin);

        size[axis1] += clamped_pref[axis1];
        size[axis2] = std::max(size[axis2], clamped_pref[axis2] + 2 * m_margin);
    }
    return size + Vector2i(0, y_offset);
}

void BoxLayout::perform_layout(NVGcontext* ctx, Widget* widget) const {
    Vector2i container_size = widget->size();
    Vector2i default_container = widget->parent() && widget->parent()->size() != Vector2i(0, 0)
        ? widget->parent()->size() : Vector2i(1000, 1000);
    if (container_size == Vector2i(0, 0)) container_size = default_container;

    int axis1 = (int)m_orientation, axis2 = ((int)m_orientation + 1) % 2;
    int position = m_margin;
    int y_offset = 0;

    const Window* window = dynamic_cast<const Window*>(widget);
    int header_adjust = 0;
    if (window && !window->title().empty()) {
        header_adjust = window->theme()->m_window_header_height - m_margin / 2;
        container_size.y() -= header_adjust;
        if (m_orientation == Orientation::Vertical) {
            position += header_adjust;
        } else {
            y_offset = header_adjust;
        }
    }

    // Collect visible children and compute total pref, min, max on axis1
    std::vector<Widget*> visible_children;
    float total_pref = 0.f;
    float total_min = 0.f;
    float total_max = 0.f;
    for (auto w : widget->children()) {
        if (w->visible()) {
            visible_children.push_back(w);
            Vector2i ps = w->preferred_size(ctx);
            Vector2i min_s = w->min_size();
            Vector2i max_s = w->max_size();

            int axis_pref = ps[axis1];
            int axis_min = min_s[axis1];
            int axis_max = max_s[axis1] > 0 ? max_s[axis1] : (container_size[axis1] - 2 * m_margin);

            total_pref += axis_pref;
            total_min += axis_min;
            total_max += axis_max;
        }
    }

    if (visible_children.empty()) return;

    float available_axis1 = container_size[axis1] - 2 * m_margin;
    int num_visible = (int)visible_children.size();
    available_axis1 -= std::max(0, num_visible - 1) * m_spacing;

    // If total_min > available, use min for each, overflow will be clipped
    bool overflow = total_min > available_axis1;
    float scale_factor = 1.0f;
    if (!overflow && total_pref > available_axis1) {
        scale_factor = available_axis1 / total_pref;
    }

    for (auto w : visible_children) {
        Vector2i ps = w->preferred_size(ctx);
        Vector2i min_s = w->min_size();
        Vector2i max_s = w->max_size();

        int axis_pref = ps[axis1];
        int axis_min = min_s[axis1];
        int axis_max = max_s[axis1] > 0 ? max_s[axis1] : (container_size[axis1] - 2 * m_margin);

        int target_axis1 = axis_pref;
        if (overflow) {
            target_axis1 = axis_min;
        } else if (scale_factor < 1.0f) {
            target_axis1 = (int)(axis_pref * scale_factor);
        }
        target_axis1 = std::max(axis_min, std::min(target_axis1, axis_max));

        Vector2i pos(0, y_offset);
        pos[axis1] = position;

        // For axis2
        int available_axis2 = container_size[axis2] - 2 * m_margin;
        int target_axis2 = ps[axis2];
        int axis2_min = min_s[axis2];
        int axis2_max = max_s[axis2] > 0 ? max_s[axis2] : available_axis2;

        switch (m_alignment) {
            case Alignment::Minimum:
                pos[axis2] += m_margin;
                break;
            case Alignment::Middle:
                pos[axis2] += (available_axis2 - target_axis2) / 2;
                break;
            case Alignment::Maximum:
                pos[axis2] += available_axis2 - target_axis2 - m_margin * 2;
                break;
            case Alignment::Fill:
                pos[axis2] += m_margin;
                target_axis2 = available_axis2;
                break;
        }

        target_axis2 = std::max(axis2_min, std::min(target_axis2, axis2_max));

        Vector2i target_size;
        target_size[axis1] = target_axis1;
        target_size[axis2] = target_axis2;

        w->set_position(pos);
        w->set_size(target_size);
        w->perform_layout(ctx);

        position += target_axis1 + m_spacing;
    }
}


Vector2i GroupLayout::preferred_size(NVGcontext* ctx, const Widget* widget) const {
    int height = m_margin, width = 2 * m_margin;

    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        height += window->theme()->m_window_header_height - m_margin / 2;

    bool first = true, indent = false;
    Vector2i default_size = widget->parent() && widget->parent()->size() != Vector2i(0, 0)
        ? widget->parent()->size() : Vector2i(1000, 1000);
    Vector2i parent_size = widget->size();
    if (parent_size == Vector2i(0, 0)) parent_size = default_size;

    for (auto c : widget->children()) {
        if (!c->visible())
            continue;
        const Label* label = dynamic_cast<const Label*>(c);
        if (!first)
            height += (label == nullptr) ? m_spacing : m_group_spacing;
        first = false;

        Vector2i ps = c->preferred_size(ctx);
        Vector2i min_s = c->min_size();
        Vector2i max_s = c->max_size();

        Vector2i clamped_pref = ps;
        clamped_pref = max(clamped_pref, min_s);
        if (max_s.x() == 0) clamped_pref.x() = std::min(clamped_pref.x(), parent_size.x() - 2 * m_margin);
        if (max_s.y() == 0) clamped_pref.y() = std::min(clamped_pref.y(), parent_size.y() - 2 * m_margin);

        bool indent_cur = indent && label == nullptr;
        height += clamped_pref.y();
        width = std::max(width, clamped_pref.x() + 2 * m_margin + (indent_cur ? m_group_indent : 0));

        if (label)
            indent = !label->caption().empty();
    }
    height += m_margin;

    return Vector2i(width, height);
}

void GroupLayout::perform_layout(NVGcontext* ctx, Widget* widget) const {
    int height = m_margin;
    int available_width = widget->width() - 2 * m_margin;
    if (available_width <= 0) {
        available_width = widget->parent() && widget->parent()->size().x() != 0
            ? widget->parent()->size().x() - 2 * m_margin : 1000 - 2 * m_margin;
    }

    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        height += window->theme()->m_window_header_height - m_margin / 2;

    bool first = true, indent = false;
    for (auto c : widget->children()) {
        if (!c->visible())
            continue;
        const Label* label = dynamic_cast<const Label*>(c);
        if (!first)
            height += (label == nullptr) ? m_spacing : m_group_spacing;
        first = false;

        bool indent_cur = indent && label == nullptr;
        Vector2i ps = Vector2i(available_width - (indent_cur ? m_group_indent : 0), c->preferred_size(ctx).y());
        Vector2i min_s = c->min_size();
        Vector2i max_s = c->max_size();

        Vector2i clamped_pref = ps;
        clamped_pref = max(clamped_pref, min_s);
        if (max_s.x() == 0) clamped_pref.x() = std::min(clamped_pref.x(), available_width - (indent_cur ? m_group_indent : 0));
        if (max_s.y() == 0) clamped_pref.y() = std::min(clamped_pref.y(), widget->height() - 2 * m_margin);

        c->set_position(Vector2i(m_margin + (indent_cur ? m_group_indent : 0), height));
        c->set_size(clamped_pref);
        c->perform_layout(ctx);

        height += clamped_pref.y();

        if (label)
            indent = !label->caption().empty();
    }
}

Vector2i GridLayout::preferred_size(NVGcontext* ctx, const Widget* widget) const {
    std::vector<int> grid[2];
    compute_layout(ctx, widget, grid);

    Vector2i size(
        2 * m_margin + std::accumulate(grid[0].begin(), grid[0].end(), 0)
        + std::max((int)grid[0].size() - 1, 0) * m_spacing[0],
        2 * m_margin + std::accumulate(grid[1].begin(), grid[1].end(), 0)
        + std::max((int)grid[1].size() - 1, 0) * m_spacing[1]
    );

	//printf("Grid computed size: %d, %d\n", size[0], size[1]);

    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        size[1] += window->theme()->m_window_header_height - m_margin / 2;

    return size;
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

    Vector2i default_size = widget->parent() && widget->parent()->size() != Vector2i(0, 0)
        ? widget->parent()->size() : Vector2i(1000, 1000);
    Vector2i parent_size = widget->size();
    if (parent_size == Vector2i(0, 0)) parent_size = default_size;

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
            Vector2i min_s = w->min_size();
            Vector2i max_s = w->max_size();

            Vector2i clamped_pref = ps;
            clamped_pref = max(clamped_pref, min_s);
            if (max_s.x() == 0) clamped_pref.x() = std::min(clamped_pref.x(), parent_size.x() - 2 * m_margin);
            if (max_s.y() == 0) clamped_pref.y() = std::min(clamped_pref.y(), parent_size.y() - 2 * m_margin);

            grid[axis1][i1] = std::max(grid[axis1][i1], clamped_pref[axis1]);
            grid[axis2][i2] = std::max(grid[axis2][i2], clamped_pref[axis2]);
        }
    }
}

void GridLayout::perform_layout(NVGcontext* ctx, Widget* widget) const {
    Vector2i default_container = widget->parent() && widget->parent()->size() != Vector2i(0, 0)
        ? widget->parent()->size() : Vector2i(1000, 1000);
    Vector2i container_size = widget->size();
    if (container_size == Vector2i(0, 0)) container_size = default_container;

    std::vector<int> grid[2];
    compute_layout(ctx, widget, grid);
    int dim[2] = { (int)grid[0].size(), (int)grid[1].size() };

    Vector2i extra(0);
    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        extra[1] += window->theme()->m_window_header_height - m_margin / 2;

    for (int i = 0; i < 2; i++) {
        int grid_size = 2 * m_margin + extra[i];
        for (int s : grid[i]) {
            grid_size += s;
            if (i + 1 < dim[i])
                grid_size += m_spacing[i];
        }

        if (grid_size < container_size[i]) {
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
            Vector2i min_s = w->min_size();
            Vector2i max_s = w->max_size();

            Vector2i clamped_pref = ps;
            clamped_pref = max(clamped_pref, min_s);
            if (max_s.x() == 0) clamped_pref.x() = std::min(clamped_pref.x(), grid[axis1][i1]);
            if (max_s.y() == 0) clamped_pref.y() = std::min(clamped_pref.y(), grid[axis2][i2]);

            Vector2i target_size = clamped_pref;

            Vector2i item_pos(pos);
            for (int j = 0; j < 2; j++) {
                int axis = (axis1 + j) % 2;
                int item = j == 0 ? i1 : i2;
                Alignment align = alignment(axis, item);
                int available = grid[axis][item];
                int axis_min = min_s[axis];
                int axis_max = max_s[axis] > 0 ? max_s[axis] : available;

                target_size[axis] = std::max(axis_min, std::min(target_size[axis], axis_max));
                target_size[axis] = std::min(target_size[axis], available);

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
                        target_size[axis] = std::max(axis_min, std::min(available, axis_max));
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
    std::vector<int> grid[2];
    compute_layout(ctx, widget, grid);

    Vector2i size(
        std::accumulate(grid[0].begin(), grid[0].end(), 0),
        std::accumulate(grid[1].begin(), grid[1].end(), 0));

    Vector2i extra(2 * m_margin);
    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        extra[1] += widget->theme()->m_window_header_height - m_margin / 2;

    return size + extra;
}

void AdvancedGridLayout::perform_layout(NVGcontext* ctx, Widget* widget) const {
    Vector2i default_container = widget->parent() && widget->parent()->size() != Vector2i(0, 0)
        ? widget->parent()->size() : Vector2i(1000, 1000);
    Vector2i container_size = widget->size();
    if (container_size == Vector2i(0, 0)) container_size = default_container;

    std::vector<int> grid[2];
    compute_layout(ctx, widget, grid);

    grid[0].insert(grid[0].begin(), m_margin);
    const Window* window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty())
        grid[1].insert(grid[1].begin(), window->theme()->m_window_header_height + m_margin / 2);
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
            int ps = w->preferred_size(ctx)[axis];
            int min_s = w->min_size()[axis];
            int max_s = w->max_size()[axis];
            int target_size = ps;
            if (max_s == 0) max_s = cell_size;
            target_size = std::max(min_s, std::min(target_size, max_s));
            target_size = std::min(target_size, cell_size);

            Vector2i pos = w->position();
            Vector2i size = w->size();
            pos[axis] = item_pos;
            size[axis] = target_size;
            w->set_position(pos);
            w->set_size(size);
            w->perform_layout(ctx);
        }
    }
}

void AdvancedGridLayout::compute_layout(NVGcontext* ctx, const Widget* widget, std::vector<int>* _grid) const {
    Vector2i default_container = widget->parent() && widget->parent()->size() != Vector2i(0, 0)
        ? widget->parent()->size() : Vector2i(1000, 1000);
    Vector2i container_size = widget->size();
    if (container_size == Vector2i(0, 0)) container_size = default_container;

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

                int ps = w->preferred_size(ctx)[axis];
                int min_s = w->min_size()[axis];
                int max_s = w->max_size()[axis];
                int target_size = ps;
                if (max_s == 0) max_s = container_size[axis];
                target_size = std::max(min_s, std::min(target_size, max_s));

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

    int y_offset = 0;
    const Window *window = dynamic_cast<const Window*>(widget);
    if (window && !window->title().empty()) {
        if (!is_row_direction())
            size[1] += widget->theme()->m_window_header_height - m_margin / 2;
        else
            y_offset = widget->theme()->m_window_header_height;
    }

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

    Vector2i default_size = widget->parent() && widget->parent()->size() != Vector2i(0, 0)
        ? widget->parent()->size() : Vector2i(1000, 1000);
    Vector2i parent_size = widget->size();
    if (parent_size == Vector2i(0, 0)) parent_size = default_size;

    // Use half parent size for ScrollPanel/Window children to avoid recursion
    Vector2i default_child_size = default_size / 2;
    for (size_t i = 0; i < visible_children.size(); ++i) {
        Widget *child = visible_children[i];
        Vector2i child_pref;
        // Use default size for ScrollPanel/Window to prevent recursion
        if (dynamic_cast<const ScrollPanel*>(child) || dynamic_cast<const Window*>(child)) {
            child_pref = default_child_size;
        } else {
            child_pref = child->preferred_size(ctx);
        }
        Vector2i min_s = child->min_size();
        Vector2i max_s = child->max_size();

        Vector2i clamped_pref = child_pref;
        clamped_pref = max(clamped_pref, min_s);
        if (max_s.x() == 0) clamped_pref.x() = std::min(clamped_pref.x(), parent_size.x() - 2 * m_margin);
        if (max_s.y() == 0) clamped_pref.y() = std::min(clamped_pref.y(), parent_size.y() - 2 * m_margin);

        FlexItem flex_item = get_flex_item(child);

        int main_size = flex_item.flex_basis >= 0 ? flex_item.flex_basis : clamped_pref[main_axis_idx];
        main_size = std::max(min_s[main_axis_idx], std::min(main_size, max_s[main_axis_idx] > 0 ? max_s[main_axis_idx] : main_size));
        total_main_size += main_size;

        int cross_size = clamped_pref[cross_axis_idx];
        cross_size = std::max(min_s[cross_axis_idx], std::min(cross_size, max_s[cross_axis_idx] > 0 ? max_s[cross_axis_idx] : cross_size));
        max_cross_size = std::max(max_cross_size, cross_size);

        if (i < visible_children.size() - 1)
            total_main_size += m_gap;
    }

    size[main_axis_idx] += total_main_size;
    size[cross_axis_idx] += max_cross_size;

    return size + Vector2i(0, y_offset);
}

void FlexLayout::perform_layout(NVGcontext *ctx, Widget *widget) const {
    Vector2i default_container = widget->parent() && widget->parent()->size() != Vector2i(0, 0)
        ? widget->parent()->size() : Vector2i(1000, 1000);
    Vector2i container_size = widget->size();
    if (container_size == Vector2i(0, 0)) container_size = default_container;

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

    std::vector<Widget*> visible_children;
    for (auto child : widget->children()) {
        if (child->visible())
            visible_children.push_back(child);
    }

    if (visible_children.empty())
        return;

    int main_axis_idx = main_axis();
    int cross_axis_idx = cross_axis();

    int available_main_space = container_size[main_axis_idx] - 2 * m_margin;
    int available_cross_space = container_size[cross_axis_idx] - 2 * m_margin;

    std::vector<int> base_sizes;
    std::vector<int> final_sizes;
    float total_flex_grow = 0.0f;
    float total_flex_shrink_scaled = 0.0f;
    int total_base_size = 0;
    int total_gaps = std::max(0, (int)visible_children.size() - 1) * m_gap;

    Vector2i default_child_size = default_container / 2; // Default for ScrollPanel/Window
    for (Widget *child : visible_children) {
        Vector2i child_pref;
        if (dynamic_cast<const ScrollPanel*>(child) || dynamic_cast<const Window*>(child)) {
            child_pref = default_child_size;
        } else {
            child_pref = child->preferred_size(ctx);
        }
        Vector2i min_s = child->min_size();
        Vector2i max_s = child->max_size();

        Vector2i clamped_pref = child_pref;
        clamped_pref = max(clamped_pref, min_s);
        if (max_s.x() == 0) clamped_pref.x() = std::min(clamped_pref.x(), available_main_space);
        if (max_s.y() == 0) clamped_pref.y() = std::min(clamped_pref.y(), available_cross_space);

        FlexItem flex_item = get_flex_item(child);

        int base_size = flex_item.flex_basis >= 0 ? flex_item.flex_basis : clamped_pref[main_axis_idx];
        int axis_min = min_s[main_axis_idx];
        int axis_max = max_s[main_axis_idx] > 0 ? max_s[main_axis_idx] : available_main_space;
        base_size = std::max(axis_min, std::min(base_size, axis_max));

        base_sizes.push_back(base_size);
        total_base_size += base_size;
        total_flex_grow += flex_item.flex_grow;
        total_flex_shrink_scaled += flex_item.flex_shrink * base_size;
    }

    int remaining_space = available_main_space - total_base_size - total_gaps;

    for (size_t i = 0; i < visible_children.size(); ++i) {
        Widget *child = visible_children[i];
        FlexItem flex_item = get_flex_item(child);
        int final_size = base_sizes[i];

        if (remaining_space > 0 && total_flex_grow > 0) {
            float grow_factor = flex_item.flex_grow / total_flex_grow;
            final_size += (int)(remaining_space * grow_factor);
        } else if (remaining_space < 0 && total_flex_shrink_scaled > 0) {
            float shrink_factor = (flex_item.flex_shrink * base_sizes[i]) / total_flex_shrink_scaled;
            final_size += (int)(remaining_space * shrink_factor);
            final_size = std::max(0, final_size);
        }

        int axis_min = child->min_size()[main_axis_idx];
        int axis_max = child->max_size()[main_axis_idx] > 0 ? child->max_size()[main_axis_idx] : available_main_space;
        final_size = std::max(axis_min, std::min(final_size, axis_max));

        final_sizes.push_back(final_size);
    }

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

    if (is_reverse_direction()) {
        std::reverse(positions.begin(), positions.end());
        std::reverse(final_sizes.begin(), final_sizes.end());
        for (size_t i = 0; i < positions.size(); ++i) {
            positions[i] = container_size[main_axis_idx] - positions[i] - final_sizes[i];
        }
    }

    for (size_t i = 0; i < visible_children.size(); ++i) {
        Widget *child = visible_children[i];
        Vector2i child_pos = child->position();
        Vector2i child_size = child->size();
        FlexItem flex_item = get_flex_item(child);

        child_pos[main_axis_idx] = positions[i];
        child_size[main_axis_idx] = final_sizes[i];

        Vector2i child_pref;
        if (dynamic_cast<const ScrollPanel*>(child) || dynamic_cast<const Window*>(child)) {
            child_pref = default_child_size;
        } else {
            child_pref = child->preferred_size(ctx);
        }
        Vector2i min_s = child->min_size();
        Vector2i max_s = child->max_size();

        AlignItems align = (flex_item.align_self != AlignItems::FlexStart) ? flex_item.align_self : m_align_items;
        int cross_size = child_pref[cross_axis_idx];
        int cross_min = min_s[cross_axis_idx];
        int cross_max = max_s[cross_axis_idx] > 0 ? max_s[cross_axis_idx] : available_cross_space;
        cross_size = std::max(cross_min, std::min(cross_size, cross_max));

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
                child_size[cross_axis_idx] = std::max(cross_min, std::min(available_cross_space, cross_max));
                break;
            case AlignItems::Baseline:
                child_pos[cross_axis_idx] = m_margin;
                child_size[cross_axis_idx] = cross_size;
                break;
        }

        if (y_offset > 0)
            child_pos.y() += y_offset;

        child->set_position(child_pos);
        child->set_size(child_size);
        child->perform_layout(ctx);
    }
}

NAMESPACE_END(nanogui)
