//
// Copyright (C) Wojciech Jarosz <wjarosz@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE.txt file.
//

#include <nanogui/menu.h>
#include <cassert>
#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/opengl.h>
#include <nanogui/popupbutton.h>
#include <nanogui/screen.h>
#include <nanogui/theme.h>
#include <nanogui/scrollpanel.h>
#include <stdexcept> // for runtime_error
#include <map>

using std::runtime_error;
using std::string;
using std::vector;

//! Platform-dependent name for the command/ctrl key
#ifdef __APPLE__
static const string CMD = "Cmd";
#else
static const string CMD = "Ctrl";
#endif

//! Platform-dependent name for the alt/option key
#ifdef __APPLE__
static const string ALT = "Opt";
#else
static const string ALT = "Alt";
#endif

namespace
{
constexpr int menu_item_height = 20;
constexpr int seperator_height = 8;
} // namespace



Shortcut::Shortcut(int m, int k) : modifiers(m), key(k)
{
    if (modifiers & SYSTEM_COMMAND_MOD)
        text += "Cmd+";;
    if (modifiers & GLFW_MOD_ALT)
        text += "Alt+";
    if (modifiers & GLFW_MOD_SHIFT)
        text += "Shift+";

    // printable characters
    if (32 < key && key < 128)
        text += char(key);
    // function keys
    else if (GLFW_KEY_F1 <= key && key <= GLFW_KEY_F25)
        //text += fmt::format("F{}", key - GLFW_KEY_F1 + 1);
        text += "F1";
    else if (GLFW_KEY_KP_0 <= key && key <= GLFW_KEY_KP_0)
        //text += fmt::format("{}", key - GLFW_KEY_KP_0);
        text += key - GLFW_KEY_KP_0;

    static const std::map<int, string> key_map = {
        {GLFW_KEY_SPACE, "Space"},
        {GLFW_KEY_ESCAPE, "Esc"},
        {GLFW_KEY_ENTER, "Enter"},
        {GLFW_KEY_TAB, "Tab"},
        {GLFW_KEY_BACKSPACE, "Backspace"},
        {GLFW_KEY_INSERT, "Insert"},
        {GLFW_KEY_DELETE, "Delete"},
        {GLFW_KEY_RIGHT, "Right"},
        {GLFW_KEY_LEFT, "Left"},
        {GLFW_KEY_DOWN, "Down"},
        {GLFW_KEY_UP, "Up"},
        {GLFW_KEY_PAGE_UP, "Page Up"},
        {GLFW_KEY_PAGE_DOWN, "Page Down"},
        {GLFW_KEY_HOME, "Home"},
        {GLFW_KEY_END, "End"},
        {GLFW_KEY_CAPS_LOCK, "Caps lock"},
        {GLFW_KEY_SCROLL_LOCK, "Scroll lock"},
        {GLFW_KEY_NUM_LOCK, "Num lock"},
        {GLFW_KEY_PRINT_SCREEN, "Print"},
        {GLFW_KEY_PAUSE, "Pause"},
        {GLFW_KEY_KP_DECIMAL, "."},
        {GLFW_KEY_KP_DIVIDE, "/"},
        {GLFW_KEY_KP_MULTIPLY, "*"},
        {GLFW_KEY_KP_SUBTRACT, "-"},
        {GLFW_KEY_KP_ADD, "+"},
        {GLFW_KEY_KP_ENTER, "Enter"},
        {GLFW_KEY_KP_EQUAL, "="},
    };

    if (auto search = key_map.find(key); search != key_map.end())
        text += search->second;
}

NAMESPACE_BEGIN(nanogui)

enum EDirection
{
    Forward,
    Backward,
};

//! Returns a modulus b.
template <typename T>
inline T mod(T a, T b)
{
    int n = (int)(a / b);
    a -= n * b;
    if (a < 0)
        a += b;
    return a;
}

int next_visible_child(const Widget *w, int start_index, EDirection direction, bool must_be_enabled)
{
    //spdlog::trace("next_visible_child({})", start_index);
    if (!w->child_count())
        return -1;

    int dir = direction == Forward ? 1 : -1;

    int found_index = start_index;
    if (!(0 <= start_index && start_index < w->child_count()))
        start_index = dir > 0 ? -1 : w->child_count();

    for (int inc = 1; inc < w->child_count(); ++inc)
    {
        int i = mod(start_index - dir * inc, w->child_count());
        if (!w->child_at(i)->visible() || !(w->child_at(i)->enabled() || !must_be_enabled))
            continue;

        found_index = i;
    }

    return found_index;
}

int nth_visible_child_index(const Widget *w, int n)
{
    if (n < 0)
        return -1;

    int last_visible = -1;
    for (int i = 0; i < w->child_count(); ++i)
    {
        if (w->child_at(i)->visible())
        {
            last_visible = i;
            if (n == 0)
                break;

            --n;
        }
    }
    return last_visible;
}

MenuItem::MenuItem(Widget *parent, const std::string &caption, int button_icon, const std::vector<Shortcut> &s) :
    Button(parent, caption, button_icon), m_shortcuts(s)
{
    set_fixed_height(menu_item_height);
    m_icon_position = IconPosition::Left;
}

Vector2i MenuItem::preferred_text_size(NVGcontext *ctx) const
{
    int font_size = m_font_size == -1 ? m_theme->m_button_font_size : m_font_size;
    nvgFontSize(ctx, font_size);
    nvgFontFace(ctx, "sans-bold");
    float tw = nvgTextBounds(ctx, 0, 0, m_caption.c_str(), nullptr, nullptr);

    return Vector2i((int)(tw) + 24, font_size + 10);
}

Vector2i MenuItem::preferred_size(NVGcontext *ctx) const
{
    int font_size = m_font_size == -1 ? m_theme->m_button_font_size : m_font_size;
    // float iw = 0.0f, ih = font_size * icon_scale();
    // ih *= icon_scale();
    // nvgFontFace(ctx, "icons");
    // nvgFontSize(ctx, ih);
    // iw = nvgTextBounds(ctx, 0, 0, utf8(m_icon).data(), nullptr, nullptr) + m_size.y() * 0.15f;
    float iw = font_size * icon_scale();
    float sw =
        shortcut().text.size() ? nvgTextBounds(ctx, 0, 0, shortcut().text.c_str(), nullptr, nullptr) + iw * 5 : 0;
    return preferred_text_size(ctx) + Vector2i((int)(iw + sw), 0);
}

void MenuItem::set_highlighted(bool highlight, bool unhighlight_siblings, bool run_callbacks)
{
    //spdlog::trace("MenuItem::set_highlighted({}, {}, {}) for \"{}\"; m_highlighted = {}", highlight,
    //              unhighlight_siblings, run_callbacks, caption(), m_highlighted);

    if (highlight != m_highlighted)
    {
        m_highlighted = highlight;
        if (run_callbacks)
        {
            if (m_highlight_callback)
                m_highlight_callback(highlight);
        }
    }

    // un-highlight siblings
    if (unhighlight_siblings)
    {
        for (auto &sibling : parent()->children())
        {
            if (sibling == this)
                continue;

            if (auto i = dynamic_cast<MenuItem *>(sibling))
            {
                // unhighlight sibling and collapse any submenus
                i->set_highlighted(false);
                if (auto dd = dynamic_cast<Dropdown *>(i))
                {
                    dd->popup()->set_visible(false);
                    dd->popup()->set_highlighted_index(-1);
                    if (dd->mode() == Dropdown::Submenu)
                        parent()->request_focus();
                }
            }
        }
    }
}

bool MenuItem::mouse_enter_event(const Vector2i &p, bool enter)
{
    Button::mouse_enter_event(p, enter);

    // on mouse enter highlight the item and unhighlight all siblings
    if (enter)
    {
        set_highlighted(m_enabled && enter, true, m_enabled && enter);
        if (m_enabled && enter)
        {
            // register the currently highlighted index in the parent popup
            if (auto p = dynamic_cast<PopupMenu *>(parent()))
                p->set_highlighted_index(p->child_index(this));

            // if this is a submenu, then show and focus it
            auto self = dynamic_cast<Dropdown *>(this);
            if (self && self->mode() == Dropdown::Submenu)
            {
                self->popup()->set_visible(true);
                self->popup()->request_focus();
            }
        }
    }
    return true;
}

void MenuItem::draw(NVGcontext *ctx)
{
    Widget::draw(ctx);

    NVGcolor grad_top = m_theme->m_button_gradient_top_unfocused;
    NVGcolor grad_bot = m_theme->m_button_gradient_bot_unfocused;

    if (m_highlighted)
    {
        grad_top = m_theme->m_button_gradient_top_focused;
        grad_bot = m_theme->m_button_gradient_bot_focused;
    }

    nvgBeginPath(ctx);

    nvgRoundedRect(ctx, m_pos.x() + 1, m_pos.y() + 1.0f, m_size.x() - 2, m_size.y() - 2,
                   m_theme->m_button_corner_radius - 1);

    if (m_background_color.w() != 0)
    {
        nvgFillColor(ctx, Color(m_background_color[0], m_background_color[1], m_background_color[2], 1.f));
        nvgFill(ctx);
        if (m_pushed)
        {
            grad_top.a = grad_bot.a = 0.8f;
        }
        else
        {
            double v   = 1 - m_background_color.w();
            grad_top.a = grad_bot.a = m_enabled ? v : v * .5f + .5f;
        }
    }

    NVGpaint bg = nvgLinearGradient(ctx, m_pos.x(), m_pos.y(), m_pos.x(), m_pos.y() + m_size.y(), grad_top, grad_bot);

    nvgFillPaint(ctx, bg);
    nvgFill(ctx);

    nvgBeginPath(ctx);
    nvgStrokeWidth(ctx, 1.0f);
    nvgRoundedRect(ctx, m_pos.x() + 0.5f, m_pos.y() + (m_pushed ? 0.5f : 1.5f), m_size.x() - 1,
                   m_size.y() - 1 - (m_pushed ? 0.0f : 1.0f), m_theme->m_button_corner_radius);
    nvgStrokeColor(ctx, m_theme->m_border_light);
    nvgStroke(ctx);

    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, m_pos.x() + 0.5f, m_pos.y() + 0.5f, m_size.x() - 1, m_size.y() - 2,
                   m_theme->m_button_corner_radius);
    nvgStrokeColor(ctx, m_theme->m_border_dark);
    nvgStroke(ctx);

    int font_size = m_font_size == -1 ? m_theme->m_button_font_size : m_font_size;
    nvgFontSize(ctx, font_size);
    nvgFontFace(ctx, "sans-bold");

    Vector2f center = Vector2f(m_pos) + Vector2f(m_size) * 0.5f;
    Vector2f text_pos(6, center.y() - 1);
    NVGcolor text_color = m_text_color.w() == 0 ? m_theme->m_text_color : m_text_color;
    if (!m_enabled)
        text_color = m_theme->m_disabled_text_color;

    auto  icon = m_icon && !m_pushed ? utf8(m_icon) : utf8(FA_CHECK);
    float ih   = font_size * icon_scale();
    nvgFontSize(ctx, ih);
    nvgFontFace(ctx, "icons");
    float iw = nvgTextBounds(ctx, 0, 0, icon.data(), nullptr, nullptr);

    if (m_caption != "")
        ih += m_size.y() * 0.15f;

    nvgFillColor(ctx, text_color);
    nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    Vector2f icon_pos(m_pos.x() + 6, center.y() - 1);

    text_pos.x() = icon_pos.x() + ih + 2;

    if (m_pushed || m_icon)
        nvgText(ctx, icon_pos.x() + (ih - iw - 3) / 2, icon_pos.y() + 1, icon.data(), nullptr);

    nvgFontSize(ctx, font_size);
    nvgFontFace(ctx, "sans");
    nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, m_theme->m_text_color_shadow);
    nvgText(ctx, text_pos.x(), text_pos.y(), m_caption.c_str(), nullptr);
    nvgFillColor(ctx, text_color);
    nvgText(ctx, text_pos.x(), text_pos.y() + 1, m_caption.c_str(), nullptr);

    if (!shortcut().text.size())
        return;

    // float    sw = nvgTextBounds(ctx, 0, 0, shortcut().text.c_str(), nullptr, nullptr);
    Vector2f hotkey_pos(m_pos.x() + m_size.x() - 8, center.y() - 1);

    nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, m_theme->m_text_color_shadow);
    nvgText(ctx, hotkey_pos.x(), hotkey_pos.y(), shortcut().text.c_str(), nullptr);
    nvgFillColor(ctx, m_theme->m_disabled_text_color);
    nvgText(ctx, hotkey_pos.x(), hotkey_pos.y() + 1, shortcut().text.c_str(), nullptr);
}

Separator::Separator(Widget *parent) : MenuItem(parent, "--separator--")
{
    set_enabled(false);
    set_fixed_height(seperator_height);
}

void Separator::draw(NVGcontext *ctx)
{
    if (!m_enabled && m_pushed)
        m_pushed = false;

    nvgBeginPath(ctx);
    nvgMoveTo(ctx, m_pos.x() + 8, m_pos.y() + m_size.y() * 0.5f);
    nvgLineTo(ctx, m_pos.x() + m_size.x() - 8, m_pos.y() + m_size.y() * 0.5f);
    nvgStrokeColor(ctx, Color(89, 255));
    nvgStrokeWidth(ctx, 1.f);
    nvgStroke(ctx);
}

PopupMenu::PopupMenu(Screen *screen, Window *parent_window, MenuItem *parent_item, bool exclusive) :
    Popup(screen, parent_window), m_parent_item(parent_item), m_exclusive(exclusive)
{
    set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 3));
    set_visible(false);

    auto flat_theme                             = new Theme(screen->nvg_context());
    flat_theme->m_standard_font_size            = 18;
    flat_theme->m_button_font_size              = 17;
    flat_theme->m_text_box_font_size            = 16;
    flat_theme->m_window_corner_radius          = 4;
    flat_theme->m_window_fill_unfocused         = Color(50, 255);
    flat_theme->m_window_fill_focused           = Color(52, 255);
    flat_theme->m_window_header_height          = 0;
    flat_theme->m_drop_shadow                   = Color(0, 100);
    flat_theme->m_button_corner_radius          = 4;
    flat_theme->m_border_light                  = flat_theme->m_transparent;
    flat_theme->m_border_dark                   = flat_theme->m_transparent;
    flat_theme->m_button_gradient_top_focused   = Color(77, 124, 233, 255);
    flat_theme->m_button_gradient_bot_focused   = flat_theme->m_button_gradient_top_focused;
    flat_theme->m_button_gradient_top_unfocused = flat_theme->m_transparent;
    flat_theme->m_button_gradient_bot_unfocused = flat_theme->m_transparent;
    flat_theme->m_button_gradient_top_pushed    = flat_theme->m_transparent;
    flat_theme->m_button_gradient_bot_pushed    = flat_theme->m_button_gradient_top_pushed;
    flat_theme->m_window_popup                  = Color(38, 255);
    flat_theme->m_text_color_shadow             = flat_theme->m_transparent;
    set_theme(flat_theme);
}

MenuItem *PopupMenu::item(int idx) const
{
    if (0 > idx || idx >= child_count())
        return nullptr;

    return (MenuItem *)child_at(idx);
}

void PopupMenu::set_highlighted_index(int idx)
{
    //spdlog::trace("PopupMenu::set_highlighted_index({}); m_highlighted_idx = {}", idx, m_highlighted_idx);

    // unhighlight the current item
    if (auto i = item(m_highlighted_idx))
        i->set_highlighted(false, false, false);

    // highlight the desired item
    if (auto i = item(idx))
        i->set_highlighted(true, true, true);

    m_highlighted_idx = idx;
}

void PopupMenu::set_selected_index(int idx)
{
    //spdlog::trace("PopupMenu::set_selected_index({})", idx);
    if (!m_exclusive)
        return;

    if (auto i = item(m_selected_idx))
        i->set_pushed(false);
    if (auto i = item(idx))
        i->set_pushed(true);

    m_selected_idx = idx;
}

bool PopupMenu::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers)
{
    //spdlog::trace("PopupMenu::mouse_button_event({}, {}, {}, {})", p, button, down, modifiers);
    //printf("PopupMenu mouse_button_event\n");
    if (Popup::mouse_button_event(p, button, down, modifiers))
    {
        // close popup and defocus all menu items
        if (down)
        {
            // only close the popup menu if we clicked on an enabled menu item
            if (auto w = find_widget(screen()->mouse_pos() - parent()->absolute_position()))
            {
                if (!w->enabled())
                    return true;
            }

            // remove mouse focus from all menu items
            for (auto &child : children()) child->mouse_enter_event(p, false);

            set_highlighted_index(-1);
            set_visible(false);

            // close the menu and any parent menus
            Window *parent_window = m_parent_window;
            while (parent_window)
            {
                if (auto i = dynamic_cast<PopupMenu *>(parent_window))
                {
                    parent_window->set_visible(false);
                    parent_window = i->m_parent_window;
                }
                else
                {
                    parent_window->request_focus();
                    parent_window = nullptr;
                }
            }
        }
        return true;
    }
    return false;
}

bool PopupMenu::keyboard_event(int key, int scancode, int action, int modifiers)
{
    try
    {
        //spdlog::trace("PopupMenu::keyboard_event({}, {}, {}, {})", key, scancode, action, modifiers);

        auto idx_backup       = m_highlighted_idx;
        auto highlighted_item = item(m_highlighted_idx);
        auto menu             = dynamic_cast<Dropdown *>(m_parent_item);
        auto menubar          = m_parent_item ? dynamic_cast<MenuBar *>(m_parent_item->parent()) : nullptr;

        if (visible() && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            if (key == GLFW_KEY_ESCAPE)
            {
                set_visible(false);
                set_highlighted_index(-1);
                m_parent_window->request_focus();
                return true;
            }
            else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER || key == GLFW_KEY_SPACE)
            {
                set_visible(false);
                set_highlighted_index(-1);

                // close the menu and any parent menus
                Window *parent_window = m_parent_window;
                while (parent_window)
                {
                    if (auto i = dynamic_cast<PopupMenu *>(parent_window))
                    {
                        parent_window->set_visible(false);
                        parent_window = i->m_parent_window;
                    }
                    else
                    {
                        parent_window->request_focus();
                        parent_window = nullptr;
                    }
                }

                // execute the item's callback
                if (auto i = item(idx_backup))
                {
                    if (i->callback())
                        i->callback()();
                    if (!(i->flags() & Button::NormalButton) && i->change_callback())
                    {
                        i->set_pushed(!i->pushed());
                        i->change_callback()(i->pushed());
                    }
                }

                return true;
            }
            else if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)
            {
                set_highlighted_index(
                    next_visible_child(this, m_highlighted_idx, key == GLFW_KEY_UP ? Backward : Forward, true));
                return true;
            }
            else if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) && m_parent_item)
            {
                if (!menu)
                    return false;

                // if we are in a submenu, collapse it when hitting the left arrow
                if (menu->mode() == Dropdown::Submenu && key == GLFW_KEY_LEFT)
                {
                    set_visible(false);
                    set_highlighted_index(-1);
                    m_parent_window->request_focus();
                    return true;
                }
                else if (menu->mode() == Dropdown::Menu || menu->mode() == Dropdown::Submenu)
                {
                    auto i = dynamic_cast<Dropdown *>(highlighted_item);

                    // if the highlighted item is a submenu, show it, focus it, and highlight the first submenu item
                    // when pressing the right arrow
                    if (i && key == GLFW_KEY_RIGHT)
                    {
                        i->popup()->set_visible(true);
                        i->popup()->request_focus();
                        i->popup()->set_highlighted_index(next_visible_child(i->popup(), -1, Forward, true));
                        return true;
                    }
                    // otherwise, if a regular menu item is highlighted in a top-level menu, left/right arrows move
                    // between top-level menus
                    else if (menubar)
                    {
                        m_parent_item->set_pushed(false);
                        m_parent_item->set_highlighted(false);
                        set_visible(false);
                        set_highlighted_index(-1);

                        int our_idx = menubar->child_index(m_parent_item);
                        int sibling_idx =
                            next_visible_child(menubar, our_idx, key == GLFW_KEY_LEFT ? Backward : Forward, true);

                        if (auto sibling_dropdown = dynamic_cast<Dropdown *>(menubar->child_at(sibling_idx)))
                        {
                            sibling_dropdown->set_pushed(true);
                            sibling_dropdown->popup()->set_visible(true);
                            sibling_dropdown->popup()->request_focus();
                            sibling_dropdown->set_highlighted(true, true, true);

                            return true;
                        }

                        return false;
                    }
                }
            }
        }

        return false;
    }
    catch (const std::exception &e)
    {
        printf("Caught an exception in PopupMenu::keyboard_event(): %s", e.what());
    }

    return false;
}

void PopupMenu::draw(NVGcontext *ctx)
{
    // refresh_relative_placement();
    m_visible &= m_parent_window->visible_recursive();

    if (!m_visible)
        return;

    int ds = m_theme->m_window_drop_shadow_size, cr = m_theme->m_window_corner_radius;

    nvgSave(ctx);
    nvgResetScissor(ctx);

    /* Draw a drop shadow */
    NVGpaint shadow_paint = nvgBoxGradient(ctx, m_pos.x(), m_pos.y() + 0.25 * ds, m_size.x(), m_size.y(), cr * 2,
                                           ds * 2, m_theme->m_drop_shadow, m_theme->m_transparent);

    nvgBeginPath(ctx);
    nvgRect(ctx, m_pos.x() - ds, m_pos.y() - ds + 0.25 * ds, m_size.x() + 2 * ds, m_size.y() + 2 * ds);
    nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr);
    nvgPathWinding(ctx, NVG_HOLE);
    nvgFillPaint(ctx, shadow_paint);
    nvgFill(ctx);

    /* Draw window */
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr);
    nvgStrokeWidth(ctx, 3.f);
    nvgStrokeColor(ctx, Color(6, 255));
    nvgStroke(ctx);
    nvgStrokeWidth(ctx, 2.f);
    nvgStrokeColor(ctx, Color(89, 255));
    nvgStroke(ctx);
    nvgFillColor(ctx, m_theme->m_window_popup);
    nvgFill(ctx);

    nvgRestore(ctx);

    Widget::draw(ctx);
}

Dropdown::Dropdown(Widget *parent, Mode mode, const string &caption) : MenuItem(parent, caption), m_mode(mode)
{
    set_flags(Flags::ToggleButton);

    m_popup = new PopupMenu(screen(), window(), this, m_mode == ComboBox);
    m_popup->set_visible(false);

    if (m_mode == Menu)
        set_fixed_size(preferred_size(screen()->nvg_context()));

    set_fixed_height(menu_item_height);
}

Dropdown::Dropdown(Widget *parent, const vector<string> &items, const vector<int> &icons, Mode mode,
                   const string &caption) :
    Dropdown(parent, mode, caption)
{
    for (int index = 0; index < (int)items.size(); ++index)
    {
        auto caption = items[index];
        auto icon    = icons.size() == items.size() ? icons[index] : 0;
        auto item    = new MenuItem{m_popup, caption, icon};
        item->set_flags(m_mode == ComboBox ? Button::RadioButton : Button::NormalButton);
        item->set_callback(
            [this, index]
            {
                set_selected_index(index);
                if (m_popup->selected_callback())
                    m_popup->selected_callback()(index);
            });
    }

    set_selected_index(0);
}

MenuItem *Dropdown::add_item(const string &caption, int icon, const vector<Shortcut> &s)
{
    auto ret = new MenuItem{popup(), caption, icon, s};
    return ret;
}

MenuItem *Dropdown::add_item(const std::pair<std::string, std::string> &item_data, int icon,
				   const std::function<void()> &callback, const std::vector<Shortcut> &shortcuts,
				   bool visible) {
	auto ret = new MenuItem{popup(), item_data.first, icon, shortcuts};
	ret->set_flags(m_mode == ComboBox ? Button::RadioButton : Button::NormalButton);
	ret->set_visible(visible);
	ret->set_callback([this, index = m_popup->child_count() - 1, callback, item_data] {
		if (callback)
			callback();
		set_selected_index(index);
		if (m_popup->selected_callback())
			m_popup->selected_callback()(index);
	});
	// Store item_data.second (ID) in MenuItem's tooltip for reference
	ret->set_tooltip(item_data.second);
	return ret;
}

Dropdown *Dropdown::add_submenu(const string &caption, int icon)
{
    auto ret = new Dropdown{popup(), Dropdown::Submenu, caption};
    ret->set_icon(icon);
    return ret;
}

void Dropdown::remove_item(int index) {
    if (index < 0 || index >= m_popup->child_count()) {
        return; // Invalid index, do nothing
    }

    // Get the MenuItem to remove
    MenuItem* item = m_popup->item(index);
    if (!item) {
        return; // Item not found
    }

    // Increment reference count to prevent deletion during event handling
    item->inc_ref();

    // Remove the item from the popup
    m_popup->remove_child(item);

    // Update selected index if necessary
    if (m_popup->selected_index() == index) {
        // Select the next valid item, or -1 if no items remain
        int new_index = -1;
        if (m_popup->child_count() > 0) {
            new_index = std::min(index, m_popup->child_count() - 1);
            if (new_index >= 0) {
                m_popup->set_selected_index(new_index);
                if (auto new_item = m_popup->item(new_index)) {
                    set_caption(new_item->caption());
                }
            }
        } else {
            m_popup->set_selected_index(-1);
            set_caption(""); // Clear caption if no items remain
        }
    } else if (m_popup->selected_index() > index) {
        // Adjust selected index if the removed item was before it
        m_popup->set_selected_index(m_popup->selected_index() - 1);
    }

    // Decrement reference count after removal
    item->dec_ref();
}

Vector2i Dropdown::preferred_size(NVGcontext *ctx) const
{
    int font_size = m_font_size == -1 ? m_theme->m_button_font_size : m_font_size;
    if (m_mode == ComboBox)
    {
        int w = 0;
        for (auto c : m_popup->children())
            if (auto i = dynamic_cast<MenuItem *>(c))
                w = std::max(w, i->preferred_text_size(ctx).x());
        return Vector2i(w + 0.5 * font_size * icon_scale(), font_size + 5);
    }
    else if (m_mode == Menu)
        return MenuItem::preferred_size(ctx) - Vector2i(4 + font_size * icon_scale(), 0);
    else
        return MenuItem::preferred_size(ctx);
}

void Dropdown::update_popup_geometry() const
{
    int      font_size = m_font_size == -1 ? m_theme->m_button_font_size : m_font_size;
    Vector2i offset;
    if (m_mode == ComboBox)
        offset = Vector2i(-3 - font_size * icon_scale(), -selected_index() * menu_item_height - 4);
    else if (m_mode == Menu)
        offset = Vector2i(0, height() + 4);
    else
        offset = Vector2i(size().x(), -4);

    Vector2i abs_pos = absolute_position() + offset;

    // prevent bottom of menu from getting clipped off screen
    abs_pos.y() += std::min(0, screen()->height() - (abs_pos.y() + m_popup->size().y() + 2));

    // prevent top of menu from getting clipped off screen
    if (abs_pos.y() <= 1)
        abs_pos.y() = absolute_position().y() + size().y() - 2;

    m_popup->set_position(abs_pos);
    m_popup->set_width(std::max(m_popup->width(), width() + int(font_size * icon_scale()) + 4));
}

bool Dropdown::mouse_enter_event(const Vector2i &p, bool enter)
{
    if (m_mode == Submenu)
        return MenuItem::mouse_enter_event(p, enter);
    else
        return Button::mouse_enter_event(p, enter);
}

bool Dropdown::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers)
{
    //spdlog::trace("Dropdown::mouse_button_event({}, {}, {}, {})", p, button, down, modifiers);
    //printf("Dropdown::mouse_button_event(%d,%d), %d, %d, %d)\n", p[0], p[1], button, down, modifiers);
    auto ret = MenuItem::mouse_button_event(p, button, down, modifiers);
    if (m_enabled && m_pushed)
    {
        if (!m_focused)
            request_focus();

        update_popup_geometry();

        // first turn focus off on all menu buttons
        for (auto it : m_popup->children()) it->mouse_enter_event(p - m_pos, false);

        // now turn focus on to just the item under the cursor
        if (auto w = m_popup->find_widget(screen()->mouse_pos() - m_popup->parent()->absolute_position()))
            w->mouse_enter_event(p + absolute_position() - w->absolute_position(), true);

        if (m_mode != ComboBox)
            m_popup->set_highlighted_index(-1);

        m_popup->set_visible(true);
        m_popup->request_focus();
    }
    else
    {
        m_popup->set_visible(false);
        m_popup->set_highlighted_index(-1);
    }
    return ret;
}

void Dropdown::draw(NVGcontext *ctx)
{
    if (!m_popup->visible())
        set_pushed(false);
    else
    {
        update_popup_geometry();
        m_popup->perform_layout(ctx);
    }

    if (!m_enabled && m_pushed)
        m_pushed = false;

    Widget::draw(ctx);

    NVGcolor grad_top = m_theme->m_button_gradient_top_unfocused;
    NVGcolor grad_bot = m_theme->m_button_gradient_bot_unfocused;

    if (m_pushed)
    {
        grad_top = m_theme->m_button_gradient_top_pushed;
        grad_bot = m_theme->m_button_gradient_bot_pushed;
    }

    if (m_mode != Submenu && m_mouse_focus && m_enabled)
    {
        grad_top = m_theme->m_button_gradient_top_focused;
        grad_bot = m_theme->m_button_gradient_bot_focused;
    }

    if (m_mode == Submenu && m_highlighted && m_enabled)
    {
        grad_top = m_theme->m_button_gradient_top_focused;
        grad_bot = m_theme->m_button_gradient_bot_focused;
    }

    nvgBeginPath(ctx);

    nvgRoundedRect(ctx, m_pos.x() + 1, m_pos.y() + 1.0f, m_size.x() - 2, m_size.y() - 2,
                   m_theme->m_button_corner_radius - 1);

    if (m_background_color.w() != 0)
    {
        nvgFillColor(ctx, Color(m_background_color[0], m_background_color[1], m_background_color[2], 1.f));
        nvgFill(ctx);
        if (m_pushed)
        {
            grad_top.a = grad_bot.a = 0.8f;
        }
        else
        {
            double v   = 1 - m_background_color.w();
            grad_top.a = grad_bot.a = m_enabled ? v : v * .5f + .5f;
        }
    }

    NVGpaint bg = nvgLinearGradient(ctx, m_pos.x(), m_pos.y(), m_pos.x(), m_pos.y() + m_size.y(), grad_top, grad_bot);

    nvgFillPaint(ctx, bg);
    nvgFill(ctx);

    nvgBeginPath(ctx);
    nvgStrokeWidth(ctx, 1.0f);
    nvgRoundedRect(ctx, m_pos.x() + 0.5f, m_pos.y() + (m_pushed ? 0.5f : 1.5f), m_size.x() - 1,
                   m_size.y() - 1 - (m_pushed ? 0.0f : 1.0f), m_theme->m_button_corner_radius);
    nvgStrokeColor(ctx, m_theme->m_border_light);
    nvgStroke(ctx);

    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, m_pos.x() + 0.5f, m_pos.y() + 0.5f, m_size.x() - 1, m_size.y() - 2,
                   m_theme->m_button_corner_radius);
    nvgStrokeColor(ctx, m_theme->m_border_dark);
    nvgStroke(ctx);

    int font_size = m_font_size == -1 ? m_theme->m_button_font_size : m_font_size;
    nvgFontSize(ctx, font_size);
    nvgFontFace(ctx, "sans-bold");

    Vector2f center = Vector2f(m_pos) + Vector2f(m_size) * 0.5f;
    Vector2f text_pos(m_pos.x() + 10, center.y() - 1);
    NVGcolor text_color = m_text_color.w() == 0 ? m_theme->m_text_color : m_text_color;
    if (!m_enabled)
        text_color = m_theme->m_disabled_text_color;

    // add an icon to the left only for submenus
    if (m_mode == Submenu)
    {
        auto  icon = utf8(m_icon);
        float ih   = font_size * icon_scale();
        nvgFontSize(ctx, ih);
        nvgFontFace(ctx, "icons");
        float iw = nvgTextBounds(ctx, 0, 0, icon.data(), nullptr, nullptr);

        ih += m_size.y() * 0.15f;

        nvgFillColor(ctx, text_color);
        nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        Vector2f icon_pos(m_pos.x() + 6, center.y() - 1);

        text_pos.x() = icon_pos.x() + ih + 2;

        if (m_icon)
            nvgText(ctx, icon_pos.x() + (ih - iw - 3) / 2, icon_pos.y() + 1, icon.data(), nullptr);
    }

    nvgFontSize(ctx, font_size);
    nvgFontFace(ctx, "sans");
    nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, m_theme->m_text_color_shadow);
    nvgText(ctx, text_pos.x(), text_pos.y(), m_caption.c_str(), nullptr);
    nvgFillColor(ctx, text_color);
    nvgText(ctx, text_pos.x(), text_pos.y() + 1, m_caption.c_str(), nullptr);

    if (m_mode != Menu)
    {
        string icon = m_mode == ComboBox ? utf8(FA_SORT) : utf8(m_theme->m_popup_chevron_right_icon);

        nvgFontSize(ctx, (m_font_size < 0 ? m_theme->m_button_font_size : m_font_size) * icon_scale());
        nvgFontFace(ctx, "icons");
        nvgFillColor(ctx, m_enabled ? text_color : NVGcolor(m_theme->m_disabled_text_color));
        nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        float    iw = nvgTextBounds(ctx, 0, 0, icon.data(), nullptr, nullptr);
        Vector2f icon_pos(0, m_pos.y() + m_size.y() * 0.5f);

        icon_pos[0] = m_pos.x() + m_size.x() - iw - 8;

        nvgText(ctx, icon_pos.x(), icon_pos.y(), icon.data(), nullptr);
    }
}

MenuBar::MenuBar(Widget *parent, const string &title) : Window(parent, title)
{
    static auto menu_theme                      = new Theme(screen()->nvg_context());
    menu_theme->m_standard_font_size            = 18;
    menu_theme->m_button_font_size              = 17;
    menu_theme->m_text_box_font_size            = 16;
    menu_theme->m_window_corner_radius          = 0;
    menu_theme->m_window_fill_unfocused         = Color(25, 255);
    menu_theme->m_window_fill_focused           = Color(25, 255);
    menu_theme->m_drop_shadow                   = Color(0, 100);
    menu_theme->m_window_header_height          = 0;
    menu_theme->m_window_drop_shadow_size       = 0;
    menu_theme->m_button_corner_radius          = 4;
    menu_theme->m_border_light                  = menu_theme->m_transparent;
    menu_theme->m_border_dark                   = menu_theme->m_transparent;
    menu_theme->m_button_gradient_top_focused   = Color(77, 124, 233, 255);
    menu_theme->m_button_gradient_bot_focused   = menu_theme->m_button_gradient_top_focused;
    menu_theme->m_button_gradient_top_pushed    = menu_theme->m_button_gradient_top_focused;
    menu_theme->m_button_gradient_bot_pushed    = menu_theme->m_button_gradient_top_focused;
    menu_theme->m_button_gradient_top_unfocused = menu_theme->m_transparent;
    menu_theme->m_button_gradient_bot_unfocused = menu_theme->m_transparent;
    menu_theme->m_window_popup                  = Color(38, 255);
    menu_theme->m_text_color_shadow             = menu_theme->m_transparent;

    set_theme(menu_theme);
    set_position(nanogui::Vector2i(0, 0));
    set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 2, 0));
}

Dropdown *MenuBar::add_menu(const string &name)
{
    auto menu = new Dropdown{this, Dropdown::Menu, name};
    menu->set_flags(Button::RadioButton);
    return menu;
}

static MenuItem *find_item_recursive(const Widget *parent, const std::vector<std::string> &menu_path, size_t index)
{
    if (index >= menu_path.size() || !parent)
        return nullptr;

    string name = menu_path[index];

    for (auto &child : parent->children())
        if (auto item = dynamic_cast<MenuItem *>(child))
            if (item->caption() == name)
            {
                if (index + 1 < menu_path.size())
                {
                    if (auto dp = dynamic_cast<Dropdown *>(item))
                        return find_item_recursive(dp->popup(), menu_path, index + 1);
                    return nullptr;
                }
                else
                    return item;
            }

    return nullptr;
}

MenuItem *MenuBar::find_item(const std::vector<std::string> &menu_path, bool throw_on_fail) const
{
    auto ret = find_item_recursive(this, menu_path, 0);
    if (ret || !throw_on_fail)
        return ret;
    else
        throw std::out_of_range("Could not find menu_path in the menu bar.");
}

bool MenuBar::mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers)
{
    // FIXME: maybe this needs to be moved to the mouse_enter_event for the child Dropdowns

    // if any menus are open, we switch menus via hover
    Dropdown *opened_menu = nullptr;
    for (auto c : children())
        if (auto d = dynamic_cast<Dropdown *>(c))
            if (d->popup()->visible())
            {
                opened_menu = d;
                break;
            }

    if (opened_menu)
    {
        auto hovered_menu = dynamic_cast<Dropdown *>(find_widget(p));
        if (hovered_menu && opened_menu != hovered_menu)
        {
            opened_menu->set_pushed(false);
            opened_menu->set_highlighted(false);
            opened_menu->popup()->set_visible(false);
            opened_menu->popup()->set_highlighted_index(-1);

            hovered_menu->set_pushed(true);
            hovered_menu->set_highlighted(true);
            hovered_menu->popup()->set_visible(true);
            hovered_menu->popup()->set_highlighted_index(-1);
            hovered_menu->popup()->request_focus();
        }
    }
    //printf("MenuBar::mouse_button_event(%d,%d) (%d,%d), %d, %d) OPENED?%s\n", p[0], p[1], rel[0], rel[1], button, modifiers, opened_menu?"true":"false");

    return Window::mouse_motion_event(p, rel, button, modifiers);
}

bool MenuBar::process_shortcuts(int modifiers, int key)
{
    Shortcut pressed{modifiers, key};
    //spdlog::trace("Checking for keyboard shortcut: \"{}\"", pressed.text);
    for (auto c : children())
        if (auto menu = dynamic_cast<Dropdown *>(c))
        {
            for (auto c2 : menu->popup()->children())
                if (auto item = dynamic_cast<MenuItem *>(c2))
                {
                    if (!item->enabled())
                        continue;
                    for (size_t i = 0; i < item->num_shortcuts(); ++i)
                        if (pressed == item->shortcut(i))
                        {
                            //spdlog::trace("Handling keyboard shortcut \"{}\" with menu item: {} > {}",
                            //              item->shortcut(i).text, menu->caption(), item->caption());
                            if (item->flags() & Button::NormalButton)
                            {
                                if (item->callback())
                                    item->callback()();
                            }
                            else
                            {
                                if (item->change_callback())
                                {
                                    item->set_pushed(!item->pushed());
                                    item->change_callback()(item->pushed());
                                }
                            }
                            return true;
                        }
                }
        }
    return false;
}

PopupWrapper::PopupWrapper(Widget *parent) : Widget(parent)
{
    m_popup = new PopupMenu{screen(), window(), nullptr, false};
    set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 0));
}

bool PopupWrapper::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers)
{
    //printf("PopupWrapper: mouse_button_event\n");
    if (m_enabled && m_popup && down)
    {
        if (button == GLFW_MOUSE_BUTTON_2)
        {
            m_popup->set_visible(true);
            m_popup->request_focus();
        }
        else
            m_popup->set_visible(false);

        m_popup->set_position(p + Vector2i(0, m_popup->size().y() - menu_item_height));
    }

    return Widget::mouse_button_event(p, button, down, modifiers);
}

NAMESPACE_END(nanogui)
