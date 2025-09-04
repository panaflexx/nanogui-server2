/*
 guieditor.h -- Header for GUIEditor class, a professional-grade GUI editor using NanoGUI.
*/

#ifndef GUIEDITOR_H
#define GUIEDITOR_H

#include <nanogui/opengl.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/checkbox.h>
#include <nanogui/textbox.h>
#include <nanogui/label.h>
#include <nanogui/combobox.h>
#include <nanogui/colorpicker.h>
#include <nanogui/textarea.h>

#include <vector>
#include <string>
#include <functional>

namespace nanogui {
    class Widget;
    class Window;
    class Button;
    class CheckBox;
    class Label;
    class TextBox;
    class ComboBox;
    class ColorPicker;
    class TextArea;
}

using namespace nanogui;
using std::vector;
using std::string;
using std::function;

class GUIEditor : public Screen {
public:
    Widget *selected_widget = nullptr;
    int current_tool = 0;
    bool dragging = false;
	Widget* original_parent = nullptr;
    Vector2i drag_start, drag_offset;
    Window *canvas_win;
    Window *editor_win;
    Widget *properties_pane;
    vector<Button *> tool_buttons;

    /* Counters for unique ID generation */
    int window_count = 0;
    int pane_count = 0;
    int label_count = 0;
    int button_count = 0;
    int textbox_count = 0;
    int combobox_count = 0;
    int checkbox_count = 0;
    int slider_count = 0;
    int colorpicker_count = 0;
    int graph_count = 0;
    int image_count = 0;

    /* Test mode toggle */
    CheckBox *test_mode_checkbox = nullptr;

    GUIEditor();

    bool update_properties();

    string getWidgetTypeName(Widget *widget);

    string generateUniqueId(int icon);

    bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override;

    bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override;

	bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override;

    bool keyboard_event(int key, int scancode, int action, int modifiers) override;

    void draw(NVGcontext *ctx) override;
private:
	bool canHaveLayout(Widget* widget);
	std::string getCurrentLayoutType(Widget* widget);
	int getLayoutTypeIndex(const std::string& type);
	void applyLayoutType(Widget* widget, int type_index);
	void addLayoutSpecificControls(Widget* widget);
	void addBoxLayoutControls(BoxLayout* layout);
	void addGridLayoutControls(GridLayout* layout);
	void addFlexLayoutControls(FlexLayout* layout);
	void addGroupLayoutControls(GroupLayout* layout);
	//void addTypeSpecificProperties(Widget* widget);
};

#endif // GUIEDITOR_H
