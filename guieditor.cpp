/*
 src/guieditor.cpp -- A professional-grade GUI editor using NanoGUI.
 Based on the provided example application.

 NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
 The widget drawing code is based on the NanoVG demo application
 by Mikko Mononen.

 All rights reserved. Use of this source code is governed by a
 BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/opengl.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/menu.h>
#include <nanogui/toolbutton.h>
#include <nanogui/popupbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/icons.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/scrollpanel.h>
#include <nanogui/colorwheel.h>
#include <nanogui/colorpicker.h>
#include <nanogui/graph.h>
#include <nanogui/tabwidget.h>
#include <nanogui/treeview.h>
#include <nanogui/texture.h>
#include <nanogui/shader.h>
#include <nanogui/renderpass.h>
#include <nanogui/textarea.h>
#include <nanogui/folderdialog.h>
#include <nanogui/icons.h> // If using Entypo, but switched to FontAwesome

#include <iostream>
#include <memory>
#include <vector>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(_MSC_VER)
# pragma warning(disable: 4505) // don't warn about dead code in stb_image.h
#elif defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <stb_image.h>

using namespace nanogui;
using std::vector;
using std::function;

class GUIEditor: public Screen {
public:
    Widget *selected_widget = nullptr;
    int current_tool = 0;
    bool dragging = false;
    Vector2i drag_start;
    Window *canvas_win;
    Window *editor_win;
    Widget *properties_pane;
    vector<Button *> tool_buttons;
    
    /* ADDED: COUNTERS FOR UNIQUE ID GENERATION */
    int window_count = 0;
    int label_count = 0;
    int button_count = 0;
    int textbox_count = 0;
    int combobox_count = 0;
    int checkbox_count = 0;
    int slider_count = 0;
    int colorpicker_count = 0;
    int graph_count = 0;
    int image_count = 0;

    void update_properties() {
        // Clear existing properties
        while (properties_pane->child_count() > 0) {
            properties_pane->remove_child(properties_pane->child_at(properties_pane->child_count() - 1));
        }

        if (!selected_widget) {
            new Label(properties_pane, "No widget selected");
            return;
        }

        /* ADDED: WIDGET TYPE DISPLAY */
        new Label(properties_pane, "Widget Type:", "sans-bold");
        TextBox *type_box = new TextBox(properties_pane);
        type_box->set_value(getWidgetTypeName(selected_widget));
        type_box->set_editable(false);
        type_box->set_fixed_height(20);

        /* ADDED: UNIQUE ID DISPLAY */
        new Label(properties_pane, "ID:", "sans-bold");
        TextBox *id_box = new TextBox(properties_pane);
        id_box->set_value(selected_widget->id());
        id_box->set_callback([this](const std::string &v) {
            selected_widget->set_id(v);
            return true;
        });
        id_box->set_fixed_height(20);

        new Label(properties_pane, "Position X:", "sans-bold");
        IntBox<int> *pos_x = new IntBox<int>(properties_pane);
        pos_x->set_value(selected_widget->position().x());
        pos_x->set_callback([this](int v) {
            Vector2i pos = selected_widget->position();
            pos.x() = v;
            selected_widget->set_position(pos);
            return true;
        });
        pos_x->set_fixed_height(20);

        new Label(properties_pane, "Position Y:", "sans-bold");
        IntBox<int> *pos_y = new IntBox<int>(properties_pane);
        pos_y->set_value(selected_widget->position().y());
        pos_y->set_callback([this](int v) {
            Vector2i pos = selected_widget->position();
            pos.y() = v;
            selected_widget->set_position(pos);
            return true;
        });
        pos_y->set_fixed_height(20);

        new Label(properties_pane, "Width:", "sans-bold");
        IntBox<int> *width_box = new IntBox<int>(properties_pane);
        width_box->set_value(selected_widget->width());
        width_box->set_callback([this](int v) {
            Vector2i size = selected_widget->size();
            size.x() = v;
            selected_widget->set_fixed_size(size);
            return true;
        });
        width_box->set_fixed_height(20);

        new Label(properties_pane, "Height:", "sans-bold");
        IntBox<int> *height_box = new IntBox<int>(properties_pane);
        height_box->set_value(selected_widget->height());
        height_box->set_callback([this](int v) {
            Vector2i size = selected_widget->size();
            size.y() = v;
            selected_widget->set_fixed_size(size);
            return true;
        });
        height_box->set_fixed_height(20);

        new Label(properties_pane, "Background Color:", "sans-bold");
        // FIXME: Not all widgets have background colors
        ColorPicker *bg_color = new ColorPicker(properties_pane); //, selected_widget->background_color());
        bg_color->set_callback([this](const Color &c) {
            //selected_widget->set_background_color(c);
        });

        // Type-specific properties
        if (Label *lbl = dynamic_cast<Label *>(selected_widget)) {
            new Label(properties_pane, "Caption:", "sans-bold");
            TextBox *caption_box = new TextBox(properties_pane);
            caption_box->set_value(lbl->caption());
            caption_box->set_callback([lbl](const std::string &v) {
                lbl->set_caption(v);
                return true;
            });
            caption_box->set_fixed_height(20);
        } else if (Button *btn = dynamic_cast<Button *>(selected_widget)) {
            new Label(properties_pane, "Caption:", "sans-bold");
            TextBox *caption_box = new TextBox(properties_pane);
            caption_box->set_value(btn->caption());
            caption_box->set_callback([btn](const std::string &v) {
                btn->set_caption(v);
                return true;
            });
            caption_box->set_fixed_height(20);
        } else if (TextBox *tb = dynamic_cast<TextBox *>(selected_widget)) {
            new Label(properties_pane, "Value:", "sans-bold");
            TextBox *value_box = new TextBox(properties_pane);
            value_box->set_value(tb->value());
            value_box->set_callback([tb](const std::string &v) {
                tb->set_value(v);
                return true;
            });
            value_box->set_fixed_height(20);
        } else if (ComboBox *cb = dynamic_cast<ComboBox *>(selected_widget)) {
            new Label(properties_pane, "Items:", "sans-bold");
            TextArea *items_area = new TextArea(properties_pane);
            std::string items_str;
            for (const auto &item : cb->items()) {
                items_str += item + "\n";
            }
            items_area->clear();
            items_area->append(items_str);
            // Note: Updating items would require parsing, omitted for simplicity
        } // Add more for other types as needed

        perform_layout();
    }

    /* ADDED: HELPER FUNCTION TO GET WIDGET TYPE NAME */
    std::string getWidgetTypeName(Widget *widget) {
        if (dynamic_cast<Window *>(widget)) return "Window";
        if (dynamic_cast<Label *>(widget)) return "Label";
        if (dynamic_cast<Button *>(widget)) return "Button";
        if (dynamic_cast<TextBox *>(widget)) return "Text Box";
        if (dynamic_cast<ComboBox *>(widget)) return "Dropdown";
        if (dynamic_cast<CheckBox *>(widget)) return "Checkbox";
        if (dynamic_cast<Slider *>(widget)) return "Slider";
        if (dynamic_cast<ColorPicker *>(widget)) return "Color Picker";
        if (dynamic_cast<Graph *>(widget)) return "Graph";
        if (dynamic_cast<Widget *>(widget)) return "Pane";
        return "Widget";
    }

    /* ADDED: HELPER FUNCTION TO GENERATE UNIQUE ID */
    std::string generateUniqueId(int icon) {
        switch (icon) {
            case FA_WINDOW_MAXIMIZE: return "WINDOW" + std::to_string(++window_count);
            case FA_TAG: return "LABEL" + std::to_string(++label_count);
            case FA_HAND_POINT_UP: return "BUTTON" + std::to_string(++button_count);
            case FA_KEYBOARD: return "TEXTBOX" + std::to_string(++textbox_count);
            case FA_CARET_DOWN: return "COMBOBOX" + std::to_string(++combobox_count);
            case FA_CHECK_SQUARE: return "CHECKBOX" + std::to_string(++checkbox_count);
            case FA_SLIDERS_H: return "SLIDER" + std::to_string(++slider_count);
            case FA_PALETTE: return "COLORPICKER" + std::to_string(++colorpicker_count);
            case FA_CHART_LINE: return "GRAPH" + std::to_string(++graph_count);
            case FA_IMAGE: return "IMAGE" + std::to_string(++image_count);
            default: return "WIDGET" + std::to_string(window_count + label_count + button_count + 1);
        }
    }

    GUIEditor(): Screen(Vector2i(1024, 768), "GUI Editor") {
        //inc_ref();
        // Editor window
        editor_win = new Window(this, "Tools & Properties");
        editor_win->set_position(Vector2i(15, 15));
        editor_win->set_layout(new BoxLayout(Orientation::Vertical));
        editor_win->set_fixed_width(250);

        // Toolbar: 4x4 grid
        Widget *toolbar = new Widget(editor_win);
        toolbar->set_layout(new GridLayout(Orientation::Horizontal, 4, Alignment::Minimum, 5, 5));

        std::vector<int> toolbar_icons = {
            FA_MOUSE_POINTER, // Select
            FA_WINDOW_MAXIMIZE, // Window
            FA_TH, // Pane (Widget)
            FA_COLUMNS, // Split
            FA_TAG, // Label
            FA_KEYBOARD, // Textbox
            FA_HAND_POINT_UP, // Button
            FA_CARET_DOWN, // Dropdown
            FA_CHECK_SQUARE, // Checkbox
            FA_SLIDERS_H, // Slider
            FA_PALETTE, // Color Picker
            FA_CHART_LINE, // Graph
            FA_IMAGE, // Image
            FA_FOLDER_OPEN, // Folder Dialog
            FA_QUESTION_CIRCLE, // Placeholder
            FA_TRASH // Delete (placeholder)
        };

        std::vector<std::string> toolbar_tooltips = {
            "Select Tool",
            "Window",
            "Widget Pane",
            "Split View",
            "Label",
            "Text Box",
            "Button",
            "Dropdown",
            "Checkbox",
            "Slider",
            "Color Picker",
            "Graph",
            "Image",
            "Folder Dialog",
            "Placeholder",
            "Delete"
        };

        for (size_t i = 0; i < toolbar_icons.size(); ++i) {
            ToolButton *tb = new ToolButton(toolbar, toolbar_icons[i]);
            tb->set_flags(Button::ToggleButton);
            tb->set_tooltip(toolbar_tooltips[i]);
            tb->set_callback([this, tb, icon = toolbar_icons[i]] {
                for (auto b : tool_buttons) {
                    if (b != tb) b->set_pushed(false);
                }
                tb->set_pushed(true);
                current_tool = icon;
            });
            tool_buttons.push_back(tb);
        }

        // Properties pane
        new Label(editor_win, "Properties", "sans-bold");
        properties_pane = new Widget(editor_win);
        properties_pane->set_layout(new GroupLayout());
        update_properties();

        // Canvas window (empty for placing widgets)
        canvas_win = new Window(this, "Canvas");
        canvas_win->set_position(Vector2i(280, 15));
        canvas_win->set_size(Vector2i(700, 700));
        canvas_win->set_layout(nullptr); // Absolute positioning

        perform_layout();
        printf("new GUIeditor complete\n");
    }

    bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        m_redraw = true; // force redraw on all mouse events

        if (button == GLFW_MOUSE_BUTTON_1) {
            Vector2i canvas_pos = p - canvas_win->position();
            if (canvas_win->contains(p - canvas_win->absolute_position())) { // Adjust for screen pos
                if (down) {
                    // Check for selection or start drag
                    bool hit_widget = false;
                    if (current_tool == FA_MOUSE_POINTER) {
                        Widget *child = canvas_win->find_widget(m_mouse_pos);
                        if (child) {
                            printf("Selected widget %s\n", child->id().c_str());
                            selected_widget = child;
                            update_properties();
                            dragging = true;
                            drag_start = p;
                            hit_widget = true;
                        }
                    }

                    if (!hit_widget && current_tool != FA_MOUSE_POINTER && current_tool != 0) {
                        // Place new widget
                        Widget *new_w = nullptr;
                        switch (current_tool) {
                            case FA_WINDOW_MAXIMIZE: {
                                Window *sub_win = new Window(canvas_win, "New Window");
                                sub_win->set_position(canvas_pos);
                                sub_win->set_fixed_size(Vector2i(200, 150));
                                sub_win->set_layout(new GroupLayout());
                                new_w = sub_win;
                                
                                /* ADDED: SET UNIQUE ID */
                                new_w->set_id(generateUniqueId(current_tool));
                            }
                            break;
                            case FA_TH: {
                                Widget *pane = new Widget(canvas_win);
                                pane->set_position(canvas_pos);
                                pane->set_fixed_size(Vector2i(150, 100));
                                pane->set_layout(new BoxLayout(Orientation::Vertical));
                                new_w = pane;
                                
                                /* ADDED: SET UNIQUE ID */
                                new_w->set_id(generateUniqueId(current_tool));
                            }
                            break;
                            case FA_COLUMNS: {
                                // Simple split simulation with two widgets side by side
                                Widget *split = new Widget(canvas_win);
                                split->set_position(canvas_pos);
                                split->set_fixed_size(Vector2i(200, 100));
                                split->set_layout(new BoxLayout(Orientation::Horizontal));
                                new Widget(split); // Left
                                new Widget(split); // Right
                                new_w = split;
                                
                                /* ADDED: SET UNIQUE ID */
                                new_w->set_id(generateUniqueId(current_tool));
                            }
                            break;
                            case FA_TAG: {
                                Label *lbl = new Label(canvas_win, "Label");
                                lbl->set_position(canvas_pos);
                                lbl->set_fixed_size(Vector2i(100, 20));
                                new_w = lbl;
                                
                                /* ADDED: SET UNIQUE ID */
                                new_w->set_id(generateUniqueId(current_tool));
                            }
                            break;
                            case FA_KEYBOARD: {
                                TextBox *tb = new TextBox(canvas_win);
                                tb->set_position(canvas_pos);
                                tb->set_fixed_size(Vector2i(150, 25));
                                tb->set_value("Text");
                                new_w = tb;
                                
                                /* ADDED: SET UNIQUE ID */
                                new_w->set_id(generateUniqueId(current_tool));
                            }
                            break;
                            case FA_HAND_POINT_UP: {
                                Button *btn = new Button(canvas_win, "Button");
                                btn->set_position(canvas_pos);
                                btn->set_fixed_size(Vector2i(100, 25));
                                new_w = btn;
                                
                                /* ADDED: SET UNIQUE ID */
                                new_w->set_id(generateUniqueId(current_tool));
                            }
                            break;
                            case FA_CARET_DOWN: {
                                ComboBox *cb = new ComboBox(canvas_win, {"Item 1", "Item 2"});
                                cb->set_position(canvas_pos);
                                cb->set_fixed_size(Vector2i(150, 25));
                                new_w = cb;
                                
                                /* ADDED: SET UNIQUE ID */
                                new_w->set_id(generateUniqueId(current_tool));
                            }
                            break;
                            case FA_CHECK_SQUARE: {
                                CheckBox *cb = new CheckBox(canvas_win, "Checkbox");
                                cb->set_position(canvas_pos);
                                cb->set_fixed_size(Vector2i(150, 25));
                                new_w = cb;
                                
                                /* ADDED: SET UNIQUE ID */
                                new_w->set_id(generateUniqueId(current_tool));
                            }
                            break;
                            case FA_SLIDERS_H: {
                                Slider *sl = new Slider(canvas_win);
                                sl->set_position(canvas_pos);
                                sl->set_fixed_size(Vector2i(150, 25));
                                new_w = sl;
                                
                                /* ADDED: SET UNIQUE ID */
                                new_w->set_id(generateUniqueId(current_tool));
                            }
                            break;
                            case FA_PALETTE: {
                                ColorPicker *cp = new ColorPicker(canvas_win, Color(255, 0, 0, 255));
                                cp->set_position(canvas_pos);
                                cp->set_fixed_size(Vector2i(100, 100));
                                new_w = cp;
                                
                                /* ADDED: SET UNIQUE ID */
                                new_w->set_id(generateUniqueId(current_tool));
                            }
                            break;
                            // Add more cases for other tools as needed
                            default:
                                break;
                        }
                        if (new_w) {
                            selected_widget = new_w;
                            update_properties();
                            perform_layout();
                            return true;
                        }
                    }
                } else {
                    dragging = false;
                }
            }
        }
        if (Screen::mouse_button_event(p, button, down, modifiers)) {
            return true;
        }
        return false;
    }

    bool mouse_motion_event(const Vector2i &p,
                            const Vector2i &rel, int button, int modifiers) override {
        if (Screen::mouse_motion_event(p, rel, button, modifiers)) {
            return true;
        }

        if (dragging && (button & (1 << GLFW_MOUSE_BUTTON_1))) {
            Vector2i delta = p - drag_start;
            Vector2i new_pos = selected_widget->position() + delta;
            selected_widget->set_position(new_pos);
            drag_start = p;
            update_properties(); // Update position in properties
            return true;
        }
        return false;
    }

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override {
        if (Screen::keyboard_event(key, scancode, action, modifiers))
            return true;
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            set_visible(false);
            return true;
        }
        return false;
    }

    virtual void draw(NVGcontext *ctx) override {
        Screen::draw(ctx);
    }
};

int main(int /* argc */, char ** /* argv */) {
    try {
        nanogui::init();

        /* scoped variables */
        {
            ref<GUIEditor> app = new GUIEditor();
            //app->dec_ref();
            app->set_visible(true);
            app->draw_all();
            nanogui::mainloop();
        }

        nanogui::shutdown();
    } catch (const std::exception &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
#if defined(_WIN32)
        MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
        std::cerr << error_msg << std::endl;
#endif
        return -1;
    } catch (...) {
        std::cerr << "Caught an unknown error!" << std::endl;
    }

    return 0;
}

