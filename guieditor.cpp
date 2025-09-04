/*
 guieditor.cpp -- A professional-grade GUI editor using NanoGUI.
 Based on the provided example application.

 NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
 The widget drawing code is based on the NanoVG demo application
 by Mikko Mononen.

 All rights reserved. Use of this source code is governed by a
 BSD-style license that can be found in the LICENSE.txt file.
*/

#include "guieditor.h"
#include <nanogui/toolbutton.h>
#include <nanogui/icons.h>
#include <nanogui/messagedialog.h>
#include <nanogui/slider.h>
#include <nanogui/colorpicker.h>
#include <nanogui/graph.h>
#include <nanogui/imagepanel.h>
#include <nanogui/folderdialog.h>
#include <nanogui/textarea.h>

#include <iostream>
#include <memory>
#include <sstream>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(_MSC_VER)
# pragma warning(disable: 4505) // don't warn about dead code in stb_image.h
#elif defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <stb_image.h>

/* NEW: TEST MODE MANAGER - SINGLETON TO TRACK TEST MODE STATE */
class TestModeManager {
private:
    bool m_testModeEnabled = false;
    static TestModeManager* instance;
    
    TestModeManager() {}
    
public:
    static TestModeManager* getInstance() {
        if (!instance) {
            instance = new TestModeManager();
        }
        return instance;
    }
    
    bool isTestModeEnabled() const { return m_testModeEnabled; }
    void setTestModeEnabled(bool enabled) { m_testModeEnabled = enabled; }
};

TestModeManager* TestModeManager::instance = nullptr;

/* NEW: BASE CLASS FOR ALL TEST-MODE WIDGETS */
class TestWidget : public Widget {
public:
    TestWidget(Widget *parent) : Widget(parent) {}
    
    /* OVERRIDE: Block input events when test mode is OFF */
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false; // Block event when test mode is off
        }
        return Widget::mouse_button_event(p, button, down, modifiers);
    }
    
    virtual bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Widget::mouse_motion_event(p, rel, button, modifiers);
    }
    
    virtual bool scroll_event(const Vector2i &p, const Vector2f &rel) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Widget::scroll_event(p, rel);
    }
    
    virtual bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Widget::mouse_drag_event(p, rel, button, modifiers);
    }
    
    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Widget::keyboard_event(key, scancode, action, modifiers);
    }
    
    virtual bool keyboard_character_event(unsigned int codepoint) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Widget::keyboard_character_event(codepoint);
    }
    
    /* OVERRIDE: Draw border (green if selected, red if test mode is OFF and not selected) */
    virtual void draw(NVGcontext *ctx) override {
        Widget::draw(ctx);
        
        GUIEditor* editor = dynamic_cast<GUIEditor*>(screen());
        Color border = (editor && editor->selected_widget == this) ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255);
        
        // Draw border only when selected or test mode is OFF
        if ((editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
            nvgSave(ctx);
            nvgBeginPath(ctx);
            nvgRect(ctx, m_pos.x() + 1, m_pos.y() + 1, m_size.x() - 1, m_size.y() - 1);
            nvgStrokeColor(ctx, border);
            nvgStrokeWidth(ctx, (editor && editor->selected_widget == this) ? 2.0f : 1.5f);
            nvgStroke(ctx);
            nvgRestore(ctx);
        }
    }
};

/* NEW: TEST-MODE SPECIFIC WIDGET CLASSES */
class TestLabel : public Label {
public:
    TestLabel(Widget *parent, const std::string &caption = "", const std::string &font = "sans") 
        : Label(parent, caption, font) {}
    
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Label::mouse_button_event(p, button, down, modifiers);
    }
    
    virtual bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Label::mouse_motion_event(p, rel, button, modifiers);
    }
    
    virtual bool scroll_event(const Vector2i &p, const Vector2f &rel) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Label::scroll_event(p, rel);
    }
    
    virtual bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Label::mouse_drag_event(p, rel, button, modifiers);
    }
    
    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Label::keyboard_event(key, scancode, action, modifiers);
    }
    
    virtual bool keyboard_character_event(unsigned int codepoint) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Label::keyboard_character_event(codepoint);
    }
    
    virtual void draw(NVGcontext *ctx) override {
        Label::draw(ctx);
        
        GUIEditor* editor = dynamic_cast<GUIEditor*>(screen());
        Color border = (editor && editor->selected_widget == this) ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255);
        
        // Draw border only when selected or test mode is OFF
        if ((editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
            nvgSave(ctx);
            nvgBeginPath(ctx);
            nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
            nvgStrokeColor(ctx, border);
            nvgStrokeWidth(ctx, (editor && editor->selected_widget == this) ? 2.0f : 1.5f);
            nvgStroke(ctx);
            nvgRestore(ctx);
        }
    }
};

class TestButton : public Button {
public:
    TestButton(Widget *parent, const std::string &caption = "", int icon = 0) 
        : Button(parent, caption, icon) {}
    
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Button::mouse_button_event(p, button, down, modifiers);
    }
    
    virtual bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Button::mouse_motion_event(p, rel, button, modifiers);
    }
    
    virtual bool scroll_event(const Vector2i &p, const Vector2f &rel) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Button::scroll_event(p, rel);
    }
    
    virtual bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Button::mouse_drag_event(p, rel, button, modifiers);
    }
    
    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Button::keyboard_event(key, scancode, action, modifiers);
    }
    
    virtual bool keyboard_character_event(unsigned int codepoint) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Button::keyboard_character_event(codepoint);
    }
    
    virtual void draw(NVGcontext *ctx) override {
        Button::draw(ctx);
        
        GUIEditor* editor = dynamic_cast<GUIEditor*>(screen());
        Color border = (editor && editor->selected_widget == this) ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255);
        
        // Draw border only when selected or test mode is OFF
        if ((editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
            nvgSave(ctx);
            nvgBeginPath(ctx);
            nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
            nvgStrokeColor(ctx, border);
            nvgStrokeWidth(ctx, (editor && editor->selected_widget == this) ? 2.0f : 1.5f);
            nvgStroke(ctx);
            nvgRestore(ctx);
        }
    }
};

class TestTextBox : public TextBox {
public:
    TestTextBox(Widget *parent) : TextBox(parent) {}
    
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return TextBox::mouse_button_event(p, button, down, modifiers);
    }
    
    virtual bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return TextBox::mouse_motion_event(p, rel, button, modifiers);
    }
    
    virtual bool scroll_event(const Vector2i &p, const Vector2f &rel) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return TextBox::scroll_event(p, rel);
    }
    
    virtual bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return TextBox::mouse_drag_event(p, rel, button, modifiers);
    }
    
    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return TextBox::keyboard_event(key, scancode, action, modifiers);
    }
    
    virtual bool keyboard_character_event(unsigned int codepoint) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return TextBox::keyboard_character_event(codepoint);
    }
    
    virtual void draw(NVGcontext *ctx) override {
        TextBox::draw(ctx);
        
        GUIEditor* editor = dynamic_cast<GUIEditor*>(screen());
        Color border = (editor && editor->selected_widget == this) ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255);
        
        // Draw border and text when selected or test mode is OFF
        if ((editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
            nvgSave(ctx);
            nvgBeginPath(ctx);
            nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
            nvgStrokeColor(ctx, border);
            nvgStrokeWidth(ctx, (editor && editor->selected_widget == this) ? 2.0f : 1.5f);
            nvgStroke(ctx);
            
            // Draw "EDIT MODE OFF" text only when not selected and test mode is OFF
            if (!(editor && editor->selected_widget == this) && !TestModeManager::getInstance()->isTestModeEnabled()) {
                nvgFontSize(ctx, 12.0f);
                nvgFontFace(ctx, "sans");
                nvgFillColor(ctx, Color(255, 0, 0, 255));
                nvgText(ctx, m_pos.x() + 5, m_pos.y() + 15, "EDIT MODE OFF", nullptr);
            }
            nvgRestore(ctx);
        }
    }
};

class TestComboBox : public ComboBox {
public:
    TestComboBox(Widget *parent, const std::vector<std::string> &items = {}) 
        : ComboBox(parent, items) {}
    
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ComboBox::mouse_button_event(p, button, down, modifiers);
    }
    
    virtual bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ComboBox::mouse_motion_event(p, rel, button, modifiers);
    }
    
    virtual bool scroll_event(const Vector2i &p, const Vector2f &rel) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ComboBox::scroll_event(p, rel);
    }
    
    virtual bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ComboBox::mouse_drag_event(p, rel, button, modifiers);
    }
    
    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ComboBox::keyboard_event(key, scancode, action, modifiers);
    }
    
    virtual bool keyboard_character_event(unsigned int codepoint) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ComboBox::keyboard_character_event(codepoint);
    }
    
    virtual void draw(NVGcontext *ctx) override {
        ComboBox::draw(ctx);
        
        GUIEditor* editor = dynamic_cast<GUIEditor*>(screen());
        Color border = (editor && editor->selected_widget == this) ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255);
        
        // Draw borders when selected or test mode is OFF
        if ((editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
            nvgSave(ctx);
            nvgBeginPath(ctx);
            nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
            nvgStrokeColor(ctx, border);
            nvgStrokeWidth(ctx, (editor && editor->selected_widget == this) ? 2.0f : 1.5f);
            nvgStroke(ctx);
            
            // Draw inner red border only when not selected and test mode is OFF
            if (!(editor && editor->selected_widget == this) && !TestModeManager::getInstance()->isTestModeEnabled()) {
                nvgBeginPath(ctx);
                nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
                nvgStrokeColor(ctx, Color(255, 0, 0, 255));
                nvgStrokeWidth(ctx, 1.0f);
                nvgStroke(ctx);
            }
            nvgRestore(ctx);
        }
    }
};

class TestCheckBox : public CheckBox {
public:
    TestCheckBox(Widget *parent, const std::string &caption = "",
                const std::function<void(bool)> &callback = {}) 
        : CheckBox(parent, caption, callback) {}
    
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return CheckBox::mouse_button_event(p, button, down, modifiers);
    }
    
    virtual bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return CheckBox::mouse_motion_event(p, rel, button, modifiers);
    }
    
    virtual bool scroll_event(const Vector2i &p, const Vector2f &rel) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return CheckBox::scroll_event(p, rel);
    }
    
    virtual bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return CheckBox::mouse_drag_event(p, rel, button, modifiers);
    }
    
    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return CheckBox::keyboard_event(key, scancode, action, modifiers);
    }
    
    virtual bool keyboard_character_event(unsigned int codepoint) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return CheckBox::keyboard_character_event(codepoint);
    }
    
    virtual void draw(NVGcontext *ctx) override {
        CheckBox::draw(ctx);
        
        GUIEditor* editor = dynamic_cast<GUIEditor*>(screen());
        Color border = (editor && editor->selected_widget == this) ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255);
        
        // Draw borders when selected or test mode is OFF
        if ((editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
            nvgSave(ctx);
            nvgBeginPath(ctx);
            nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
            nvgStrokeColor(ctx, border);
            nvgStrokeWidth(ctx, (editor && editor->selected_widget == this) ? 2.0f : 1.5f);
            nvgStroke(ctx);
            
            // Draw red circle only when not selected and test mode is OFF
            if (!(editor && editor->selected_widget == this) && !TestModeManager::getInstance()->isTestModeEnabled()) {
                nvgBeginPath(ctx);
                nvgCircle(ctx, m_pos.x() + 10, m_pos.y() + 10, 8);
                nvgStrokeColor(ctx, Color(255, 0, 0, 255));
                nvgStrokeWidth(ctx, 1.5f);
                nvgStroke(ctx);
            }
            nvgRestore(ctx);
        }
    }
};

class TestSlider : public Slider {
public:
    TestSlider(Widget *parent) : Slider(parent) {}
    
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Slider::mouse_button_event(p, button, down, modifiers);
    }
    
    virtual bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Slider::mouse_motion_event(p, rel, button, modifiers);
    }
    
    virtual bool scroll_event(const Vector2i &p, const Vector2f &rel) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Slider::scroll_event(p, rel);
    }
    
    virtual bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Slider::mouse_drag_event(p, rel, button, modifiers);
    }
    
    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Slider::keyboard_event(key, scancode, action, modifiers);
    }
    
    virtual bool keyboard_character_event(unsigned int codepoint) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Slider::keyboard_character_event(codepoint);
    }
    
    virtual void draw(NVGcontext *ctx) override {
        Slider::draw(ctx);
        
        GUIEditor* editor = dynamic_cast<GUIEditor*>(screen());
        Color border = (editor && editor->selected_widget == this) ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255);
        
        // Draw borders when selected or test mode is OFF
        if ((editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
            nvgSave(ctx);
            nvgBeginPath(ctx);
            nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
            nvgStrokeColor(ctx, border);
            nvgStrokeWidth(ctx, (editor && editor->selected_widget == this) ? 2.0f : 1.5f);
            nvgStroke(ctx);
            
            // Draw extra red border only when not selected and test mode is OFF
            if (!(editor && editor->selected_widget == this) && !TestModeManager::getInstance()->isTestModeEnabled()) {
                nvgBeginPath(ctx);
                nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
                nvgStrokeColor(ctx, Color(255, 0, 0, 255));
                nvgStrokeWidth(ctx, 1.5f);
                nvgStroke(ctx);
            }
            nvgRestore(ctx);
        }
    }
};

class TestColorPicker : public ColorPicker {
public:
    TestColorPicker(Widget *parent, const Color &color = Color(0, 255)) 
        : ColorPicker(parent, color) {}
    
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ColorPicker::mouse_button_event(p, button, down, modifiers);
    }
    
    virtual bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false; // Fixed: Added return statement
        }
        return ColorPicker::mouse_motion_event(p, rel, button, modifiers);
    }
    
    virtual bool scroll_event(const Vector2i &p, const Vector2f &rel) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ColorPicker::scroll_event(p, rel);
    }
    
    virtual bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ColorPicker::mouse_drag_event(p, rel, button, modifiers);
    }
    
    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ColorPicker::keyboard_event(key, scancode, action, modifiers);
    }
    
    virtual bool keyboard_character_event(unsigned int codepoint) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return ColorPicker::keyboard_character_event(codepoint);
    }
    
    virtual void draw(NVGcontext *ctx) override {
        ColorPicker::draw(ctx);
        
        GUIEditor* editor = dynamic_cast<GUIEditor*>(screen());
        Color border = (editor && editor->selected_widget == this) ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255);
        
        // Draw borders and text when selected or test mode is OFF
        if ((editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
            nvgSave(ctx);
            nvgBeginPath(ctx);
            nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
            nvgStrokeColor(ctx, border);
            nvgStrokeWidth(ctx, (editor && editor->selected_widget == this) ? 2.0f : 1.5f);
            nvgStroke(ctx);
            
            // Draw extra red border and text only when not selected and test mode is OFF
            if (!(editor && editor->selected_widget == this) && !TestModeManager::getInstance()->isTestModeEnabled()) {
                nvgBeginPath(ctx);
                nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), 4);
                nvgStrokeColor(ctx, Color(255, 0, 0, 255));
                nvgStrokeWidth(ctx, 1.5f);
                nvgStroke(ctx);
                
                nvgFontSize(ctx, 12.0f);
                nvgFontFace(ctx, "sans");
                nvgFillColor(ctx, Color(255, 255, 255, 255));
                nvgTextAlign(ctx, NVG_ALIGN_CENTER);
                nvgText(ctx, m_pos.x() + m_size.x() / 2, m_pos.y() + m_size.y() / 2 + 15, "DISABLED", nullptr);
            }
            nvgRestore(ctx);
        }
    }
};

class TestWindow : public Window {
public:
    TestWindow(Widget *parent, const std::string &title = "", bool modal = false) 
        : Window(parent, title, modal) {}
    
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Window::mouse_button_event(p, button, down, modifiers);
    }
    
    virtual bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Window::mouse_motion_event(p, rel, button, modifiers);
    }
    
    virtual bool scroll_event(const Vector2i &p, const Vector2f &rel) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Window::scroll_event(p, rel);
    }
    
    virtual bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Window::mouse_drag_event(p, rel, button, modifiers);
    }
    
    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Window::keyboard_event(key, scancode, action, modifiers);
    }
    
    virtual bool keyboard_character_event(unsigned int codepoint) override {
        if (!TestModeManager::getInstance()->isTestModeEnabled()) {
            return false;
        }
        return Window::keyboard_character_event(codepoint);
    }
    
    virtual void draw(NVGcontext *ctx) override {
        Window::draw(ctx);
        
        GUIEditor* editor = dynamic_cast<GUIEditor*>(screen());
        Color border = (editor && editor->selected_widget == this) ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255);
        
        // Draw border and text when selected or test mode is OFF
        if ((editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
            nvgSave(ctx);
            nvgBeginPath(ctx);
            nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
            nvgStrokeColor(ctx, border);
            nvgStrokeWidth(ctx, (editor && editor->selected_widget == this) ? 2.0f : 2.0f);
            nvgStroke(ctx);
            
            // Draw text only when not selected and test mode is OFF
            if (!(editor && editor->selected_widget == this) && !TestModeManager::getInstance()->isTestModeEnabled()) {
                nvgFontSize(ctx, 14.0f);
                nvgFontFace(ctx, "sans");
                nvgFillColor(ctx, Color(255, 255, 255, 255));
                nvgTextAlign(ctx, NVG_ALIGN_CENTER);
                nvgText(ctx, m_pos.x() + m_size.x() / 2, m_pos.y() + 20, "TEST MODE: OFF", nullptr);
            }
            nvgRestore(ctx);
        }
    }
};

/* GUIEditor Implementations */
GUIEditor::GUIEditor() : Screen(Vector2i(1024, 768), "GUI Editor") {
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

    // Test mode toggle
    Widget *testModeRow = new Widget(editor_win);
    testModeRow->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 0, 5));
    
    test_mode_checkbox = new CheckBox(testModeRow, "Test Mode");
    test_mode_checkbox->set_callback([this](bool checked) {
        selected_widget = nullptr; // deselect current widget focus
        TestModeManager::getInstance()->setTestModeEnabled(checked);
        
        // Force redraw to update all widgets' appearance
        perform_layout();
        draw_all();
    });
    
    // Set initial state
    test_mode_checkbox->set_checked(false);
    TestModeManager::getInstance()->setTestModeEnabled(false);

    // Properties pane
    new Label(editor_win, "Properties", "sans-bold");
    properties_pane = new Widget(editor_win);
    GridLayout* layout = new GridLayout(Orientation::Horizontal, 2, 
                                       Alignment::Middle, 15, 5);
    layout->set_col_alignment({ Alignment::Maximum, Alignment::Fill });
    layout->set_spacing(Orientation::Horizontal, 10);
    properties_pane->set_layout(layout);

    update_properties();

    // Canvas window (empty for placing widgets)
    canvas_win = new Window(this, "Canvas");
    canvas_win->set_position(Vector2i(280, 15));
    canvas_win->set_size(Vector2i(700, 700));
    canvas_win->set_layout(nullptr); // Absolute positioning

    perform_layout();
}

bool GUIEditor::update_properties() {
    // Clear existing properties
    while (properties_pane->child_count() > 0) {
        properties_pane->remove_child(properties_pane->child_at(properties_pane->child_count() - 1));
    }

    if (!selected_widget) {
        new Label(properties_pane, "No widget selected");
        perform_layout();
        redraw();
        return false;
    }

    /* WIDGET TYPE DISPLAY */
    new Label(properties_pane, "Widget:", "sans-bold");
    TextBox *type_box = new TextBox(properties_pane);
    type_box->set_value(getWidgetTypeName(selected_widget));
    type_box->set_editable(false);
    type_box->set_fixed_height(20);

    /* UNIQUE ID DISPLAY */
    new Label(properties_pane, "ID:", "sans-bold");
    TextBox *id_box = new TextBox(properties_pane);
    id_box->set_value(selected_widget->id());
    id_box->set_callback([this](const std::string &v) {
		if(!selected_widget) return false;
        selected_widget->set_id(v);
        perform_layout();
        redraw();
        return true;
    });
    id_box->set_fixed_height(20);

    /* TYPE-SPECIFIC TEXT PROPERTIES */
    if (Label *lbl = dynamic_cast<Label *>(selected_widget)) {
        new Label(properties_pane, "Caption:", "sans-bold");
        TextBox *caption_box = new TextBox(properties_pane);
        caption_box->set_value(lbl->caption());
        caption_box->set_callback([this, lbl](const std::string &v) {
			if(!selected_widget) return false;
            lbl->set_caption(v);
            selected_widget->perform_layout(m_nvg_context);
            perform_layout();
            redraw();
            return true;
        });
        caption_box->set_fixed_height(20);
    } else if (Button *btn = dynamic_cast<Button *>(selected_widget)) {
        new Label(properties_pane, "Caption:", "sans-bold");
        TextBox *caption_box = new TextBox(properties_pane);
        caption_box->set_value(btn->caption());
        caption_box->set_callback([this, btn](const std::string &v) {
			if(!selected_widget) return false;
            btn->set_caption(v);
            selected_widget->perform_layout(m_nvg_context);
            perform_layout();
            redraw();
            return true;
        });
        caption_box->set_fixed_height(20);
    } else if (CheckBox *cb = dynamic_cast<CheckBox *>(selected_widget)) {
        new Label(properties_pane, "Caption:", "sans-bold");
        TextBox *caption_box = new TextBox(properties_pane);
        caption_box->set_value(cb->caption());
        caption_box->set_callback([this, cb](const std::string &v) {
			if(!selected_widget) return false;
            cb->set_caption(v);
            selected_widget->perform_layout(m_nvg_context);
            perform_layout();
            redraw();
            return true;
        });
        caption_box->set_fixed_height(20);
    } else if (Window *win = dynamic_cast<Window *>(selected_widget)) {
        new Label(properties_pane, "Title:", "sans-bold");
        TextBox *title_box = new TextBox(properties_pane);
        title_box->set_value(win->title());
        title_box->set_callback([this, win](const std::string &v) {
			if(!selected_widget) return false;
            win->set_title(v);
            selected_widget->perform_layout(m_nvg_context);
            perform_layout();
            redraw();
            return true;
        });
        title_box->set_fixed_height(20);
    } else if (TextBox *tb = dynamic_cast<TextBox *>(selected_widget)) {
        new Label(properties_pane, "Value:", "sans-bold");
        TextBox *value_box = new TextBox(properties_pane);
        value_box->set_value(tb->value());
        value_box->set_callback([this, tb](const std::string &v) {
			if(!selected_widget) return false;
            tb->set_value(v);
            selected_widget->perform_layout(m_nvg_context);
            perform_layout();
            redraw();
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
		// TODO: TextArea doesn't have a callback - should add one when user hits enter?
		/*
        items_area->set_callback([this, cb](const std::string &v) {
            std::vector<std::string> new_items;
            std::stringstream ss(v);
            std::string item;
            while (std::getline(ss, item)) {
                if (!item.empty()) {
                    new_items.push_back(item);
                }
            }
            cb->set_items(new_items);
            selected_widget->perform_layout(m_nvg_context);
            perform_layout();
            redraw();
            return true;
        });
		*/
        items_area->set_fixed_size(Vector2i(0, 60)); // Fixed height for TextArea
    }

    /* LAYOUT CONTROLS - Only for container widgets */
    if (canHaveLayout(selected_widget)) {
        new Label(properties_pane, "Layout:", "sans-bold");
        ComboBox *layout_combo = new ComboBox(properties_pane, {
            "None", "Box Layout", "Grid Layout", "Advanced Grid", "Flex Layout", "Group Layout"
        });
        
        // Set current layout type
        std::string current_layout = getCurrentLayoutType(selected_widget);
        layout_combo->set_selected_index(getLayoutTypeIndex(current_layout));
        
        layout_combo->set_callback([this](int index) {
            applyLayoutType(selected_widget, index);
            update_properties(); // Refresh to show layout-specific controls
        });
        layout_combo->set_fixed_height(20);

        // Layout-specific parameters
        addLayoutSpecificControls(selected_widget);
    }

    /* POSITION X */
    new Label(properties_pane, "Position X:", "sans-bold");
    IntBox<int> *pos_x = new IntBox<int>(properties_pane);
    pos_x->set_value(selected_widget->position().x());
    pos_x->set_callback([this](int v) {
        Vector2i pos = selected_widget->position();
        pos.x() = v;
        selected_widget->set_position(pos);
        selected_widget->perform_layout(m_nvg_context);
        perform_layout();
        redraw();
        return true;
    });
    pos_x->set_fixed_height(20);

    /* POSITION Y */
    new Label(properties_pane, "Position Y:", "sans-bold");
    IntBox<int> *pos_y = new IntBox<int>(properties_pane);
    pos_y->set_value(selected_widget->position().y());
    pos_y->set_callback([this](int v) {
        Vector2i pos = selected_widget->position();
        pos.y() = v;
        selected_widget->set_position(pos);
        selected_widget->perform_layout(m_nvg_context);
        perform_layout();
        redraw();
        return true;
    });
    pos_y->set_fixed_height(20);

    /* WIDTH */
    new Label(properties_pane, "Width:", "sans-bold");
    IntBox<int> *width_box = new IntBox<int>(properties_pane);
    width_box->set_value(selected_widget->width());
    width_box->set_callback([this](int v) {
        Vector2i size = selected_widget->size();
        size.x() = v;
        selected_widget->set_fixed_size(size);
        selected_widget->perform_layout(m_nvg_context);
        perform_layout();
        redraw();
        return true;
    });
    width_box->set_fixed_height(20);

    /* HEIGHT */
    new Label(properties_pane, "Height:", "sans-bold");
    IntBox<int> *height_box = new IntBox<int>(properties_pane);
    height_box->set_value(selected_widget->height());
    height_box->set_callback([this](int v) {
        Vector2i size = selected_widget->size();
        size.y() = v;
        selected_widget->set_fixed_size(size);
        selected_widget->perform_layout(m_nvg_context);
        perform_layout();
        redraw();
        return true;
    });
    height_box->set_fixed_height(20);

    /* BACKGROUND COLOR */
    new Label(properties_pane, "BG Color:", "sans-bold");
    ColorPicker *bg_color = new ColorPicker(properties_pane);
    bg_color->set_callback([this](const Color &c) {
        // Not all widgets support background color; placeholder for future implementation
        (void)c;
        perform_layout();
        redraw();
        return true;
    });
    bg_color->set_fixed_height(20);

    perform_layout();
    redraw();
    return true;
}

bool GUIEditor::canHaveLayout(Widget* widget) {
    return dynamic_cast<Window*>(widget) || 
           dynamic_cast<TestWindow*>(widget) ||
           dynamic_cast<TestWidget*>(widget) ||
           (widget != canvas_win && widget->child_count() > 0);
}

std::string GUIEditor::getCurrentLayoutType(Widget* widget) {
    Layout* layout = widget->layout();
    if (!layout) return "None";
    
    if (dynamic_cast<BoxLayout*>(layout)) return "Box Layout";
    if (dynamic_cast<GridLayout*>(layout)) return "Grid Layout";
    if (dynamic_cast<AdvancedGridLayout*>(layout)) return "Advanced Grid";
    if (dynamic_cast<FlexLayout*>(layout)) return "Flex Layout";
    if (dynamic_cast<GroupLayout*>(layout)) return "Group Layout";
    
    return "Unknown";
}

int GUIEditor::getLayoutTypeIndex(const std::string& type) {
    if (type == "None") return 0;
    if (type == "Box Layout") return 1;
    if (type == "Grid Layout") return 2;
    if (type == "Advanced Grid") return 3;
    if (type == "Flex Layout") return 4;
    if (type == "Group Layout") return 5;
    return 0;
}

void GUIEditor::applyLayoutType(Widget* widget, int type_index) {
    Layout* new_layout = nullptr;
    
    switch (type_index) {
        case 0: // None
            new_layout = nullptr;
            break;
        case 1: // Box Layout
            new_layout = new BoxLayout(Orientation::Vertical, Alignment::Fill, 10, 5);
            break;
        case 2: // Grid Layout
            new_layout = new GridLayout(Orientation::Horizontal, 2, Alignment::Fill, 10, 5);
            break;
        case 3: // Advanced Grid
            new_layout = new AdvancedGridLayout({100, 100}, {30, 30}, 10);
            break;
        case 4: // Flex Layout
            new_layout = new FlexLayout(FlexDirection::Column, JustifyContent::FlexStart, 
                                      AlignItems::Stretch, 10, 5);
            break;
        case 5: // Group Layout
            new_layout = new GroupLayout(10, 5, 15, 5);
            break;
    }
    
    widget->set_layout(new_layout);
    widget->perform_layout(m_nvg_context);
    // Ensure parent widgets update their layout
    if (Widget* parent = widget->parent()) {
        parent->perform_layout(m_nvg_context);
    }
    perform_layout();
    redraw();
}

void GUIEditor::addLayoutSpecificControls(Widget* widget) {
    Layout* layout = widget->layout();
    if (!layout) return;

    // Box Layout Controls
    if (BoxLayout* box_layout = dynamic_cast<BoxLayout*>(layout)) {
        addBoxLayoutControls(box_layout);
    }
    // Grid Layout Controls  
    else if (GridLayout* grid_layout = dynamic_cast<GridLayout*>(layout)) {
        addGridLayoutControls(grid_layout);
    }
    // Flex Layout Controls
    else if (FlexLayout* flex_layout = dynamic_cast<FlexLayout*>(layout)) {
        addFlexLayoutControls(flex_layout);
    }
    // Group Layout Controls
    else if (GroupLayout* group_layout = dynamic_cast<GroupLayout*>(layout)) {
        addGroupLayoutControls(group_layout);
    }
}

void GUIEditor::addBoxLayoutControls(BoxLayout* layout) {
    // Orientation
    new Label(properties_pane, "Orientation:", "sans-bold");
    ComboBox *orientation_combo = new ComboBox(properties_pane, {"Horizontal", "Vertical"});
    orientation_combo->set_selected_index(layout->orientation() == Orientation::Horizontal ? 0 : 1);
    orientation_combo->set_callback([this, layout](int index) {
        layout->set_orientation(index == 0 ? Orientation::Horizontal : Orientation::Vertical);
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
    });
    orientation_combo->set_fixed_height(20);

    // Alignment
    new Label(properties_pane, "Alignment:", "sans-bold");
    ComboBox *align_combo = new ComboBox(properties_pane, {"Minimum", "Middle", "Maximum", "Fill"});
    align_combo->set_selected_index((int)layout->alignment());
    align_combo->set_callback([this, layout](int index) {
        layout->set_alignment((Alignment)index);
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
    });
    align_combo->set_fixed_height(20);

    // Margin
    new Label(properties_pane, "Margin:", "sans-bold");
    IntBox<int> *margin_box = new IntBox<int>(properties_pane);
    margin_box->set_value(layout->margin());
    margin_box->set_callback([this, layout](int v) {
        layout->set_margin(v);
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
        return true;
    });
    margin_box->set_fixed_height(20);

    // Spacing
    new Label(properties_pane, "Spacing:", "sans-bold");
    IntBox<int> *spacing_box = new IntBox<int>(properties_pane);
    spacing_box->set_value(layout->spacing());
    spacing_box->set_callback([this, layout](int v) {
        layout->set_spacing(v);
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
        return true;
    });
    spacing_box->set_fixed_height(20);
}

void GUIEditor::addGridLayoutControls(GridLayout* layout) {
    // Resolution (columns/rows)
    new Label(properties_pane, "Resolution:", "sans-bold");
    IntBox<int> *resolution_box = new IntBox<int>(properties_pane);
    resolution_box->set_value(layout->resolution());
    resolution_box->set_callback([this, layout](int v) {
        layout->set_resolution(std::max(1, v));
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
        return true;
    });
    resolution_box->set_fixed_height(20);

    // Orientation  
    new Label(properties_pane, "Orientation:", "sans-bold");
    ComboBox *orientation_combo = new ComboBox(properties_pane, {"Horizontal", "Vertical"});
    orientation_combo->set_selected_index(layout->orientation() == Orientation::Horizontal ? 0 : 1);
    orientation_combo->set_callback([this, layout](int index) {
        layout->set_orientation(index == 0 ? Orientation::Horizontal : Orientation::Vertical);
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
    });
    orientation_combo->set_fixed_height(20);
}

void GUIEditor::addFlexLayoutControls(FlexLayout* layout) {
    // Flex Direction
    new Label(properties_pane, "Direction:", "sans-bold");
    ComboBox *direction_combo = new ComboBox(properties_pane, {
        "Row", "Row Reverse", "Column", "Column Reverse"
    });
    direction_combo->set_selected_index((int)layout->direction());
    direction_combo->set_callback([this, layout](int index) {
        layout->set_direction((FlexDirection)index);
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
    });
    direction_combo->set_fixed_height(20);

    // Justify Content
    new Label(properties_pane, "Justify:", "sans-bold");
    ComboBox *justify_combo = new ComboBox(properties_pane, {
        "Flex Start", "Flex End", "Center", "Space Between", "Space Around", "Space Evenly"
    });
    justify_combo->set_selected_index((int)layout->justify_content());
    justify_combo->set_callback([this, layout](int index) {
        layout->set_justify_content((JustifyContent)index);
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
    });
    justify_combo->set_fixed_height(20);

    // Align Items
    new Label(properties_pane, "Align Items:", "sans-bold");
    ComboBox *align_combo = new ComboBox(properties_pane, {
        "Flex Start", "Flex End", "Center", "Stretch", "Baseline"
    });
    align_combo->set_selected_index((int)layout->align_items());
    align_combo->set_callback([this, layout](int index) {
        layout->set_align_items((AlignItems)index);
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
    });
    align_combo->set_fixed_height(20);
}

void GUIEditor::addGroupLayoutControls(GroupLayout* layout) {
    // Margin
    new Label(properties_pane, "Margin:", "sans-bold");
    IntBox<int> *margin_box = new IntBox<int>(properties_pane);
    margin_box->set_value(layout->margin());
    margin_box->set_callback([this, layout](int v) {
        layout->set_margin(v);
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
        return true;
    });
    margin_box->set_fixed_height(20);

    // Spacing
    new Label(properties_pane, "Spacing:", "sans-bold");
    IntBox<int> *spacing_box = new IntBox<int>(properties_pane);
    spacing_box->set_value(layout->spacing());
    spacing_box->set_callback([this, layout](int v) {
        layout->set_spacing(v);
        selected_widget->perform_layout(m_nvg_context);
        if (Widget* parent = selected_widget->parent()) {
            parent->perform_layout(m_nvg_context);
        }
        perform_layout();
        redraw();
        return true;
    });
    spacing_box->set_fixed_height(20);
}

std::string GUIEditor::getWidgetTypeName(Widget *widget) {
    if (dynamic_cast<TestWindow *>(widget)) return "Window";
    if (dynamic_cast<TestLabel *>(widget)) return "Label";
    if (dynamic_cast<TestButton *>(widget)) return "Button";
    if (dynamic_cast<TestTextBox *>(widget)) return "Text Box";
    if (dynamic_cast<TestComboBox *>(widget)) return "Dropdown";
    if (dynamic_cast<TestCheckBox *>(widget)) return "Checkbox";
    if (dynamic_cast<TestSlider *>(widget)) return "Slider";
    if (dynamic_cast<TestColorPicker *>(widget)) return "Color Picker";
    if (dynamic_cast<TestWidget *>(widget)) return "Pane";
    return "Widget";
}

std::string GUIEditor::generateUniqueId(int icon) {
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

bool GUIEditor::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) {
    m_redraw = true; // force redraw on all mouse events

    if (Screen::mouse_button_event(p, button, down, modifiers)) {
        return true;
    }

    if (!TestModeManager::getInstance()->isTestModeEnabled() && button == GLFW_MOUSE_BUTTON_1) {
        Vector2i canvas_pos = p - canvas_win->position();
        if (canvas_win->contains(p)) { // Contains checks constraints
            if (down) {
                // Check for selection or start drag
                bool hit_widget = false;
                if (current_tool == FA_MOUSE_POINTER) {
                    // Find widget in test mode aware manner
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
                    // Place new widget using test-aware versions
                    Widget *new_w = nullptr;
                    switch (current_tool) {
                        case FA_WINDOW_MAXIMIZE: {
                            TestWindow *sub_win = new TestWindow(canvas_win, "New Window");
                            sub_win->set_position(canvas_pos);
                            sub_win->set_size(Vector2i(200, 150));
                            sub_win->set_layout(new GroupLayout());
                            new_w = sub_win;
                            
                            /* ADDED: SET UNIQUE ID */
                            new_w->set_id(generateUniqueId(current_tool));
                        }
                        break;
                        case FA_TAG: {
                            TestLabel *lbl = new TestLabel(canvas_win, "Label");
                            lbl->set_position(canvas_pos);
                            lbl->set_fixed_size(Vector2i(100, 20));
                            new_w = lbl;
                            
                            /* ADDED: SET UNIQUE ID */
                            new_w->set_id(generateUniqueId(current_tool));
                        }
                        break;
                        case FA_HAND_POINT_UP: {
                            TestButton *btn = new TestButton(canvas_win, "Button");
                            btn->set_position(canvas_pos);
                            btn->set_fixed_size(Vector2i(100, 25));
                            new_w = btn;
                            
                            /* ADDED: SET UNIQUE ID */
                            new_w->set_id(generateUniqueId(current_tool));
                        }
                        break;
                        case FA_KEYBOARD: {
                            TestTextBox *tb = new TestTextBox(canvas_win);
                            tb->set_position(canvas_pos);
                            tb->set_fixed_size(Vector2i(150, 25));
                            tb->set_value("Text");
                            new_w = tb;
                            
                            /* ADDED: SET UNIQUE ID */
                            new_w->set_id(generateUniqueId(current_tool));
                        }
                        break;
                        case FA_CARET_DOWN: {
                            TestComboBox *cb = new TestComboBox(canvas_win, {"Item 1", "Item 2"});
                            cb->set_position(canvas_pos);
                            cb->set_fixed_size(Vector2i(150, 25));
                            new_w = cb;
                            
                            /* ADDED: SET UNIQUE ID */
                            new_w->set_id(generateUniqueId(current_tool));
                        }
                        break;
                        case FA_CHECK_SQUARE: {
                            TestCheckBox *cb = new TestCheckBox(canvas_win, "Checkbox");
                            cb->set_position(canvas_pos);
                            cb->set_fixed_size(Vector2i(150, 25));
                            new_w = cb;
                            
                            /* ADDED: SET UNIQUE ID */
                            new_w->set_id(generateUniqueId(current_tool));
                        }
                        break;
                        case FA_SLIDERS_H: {
                            TestSlider *sl = new TestSlider(canvas_win);
                            sl->set_position(canvas_pos);
                            sl->set_fixed_size(Vector2i(150, 25));
                            new_w = sl;
                            
                            /* ADDED: SET UNIQUE ID */
                            new_w->set_id(generateUniqueId(current_tool));
                        }
                        break;
                        case FA_PALETTE: {
                            TestColorPicker *cp = new TestColorPicker(canvas_win, Color(255, 0, 0, 255));
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
                        redraw();
                        return true;
                    }
                }
            } else {
                dragging = false;
            }
        }
    }

    return false;
}

bool GUIEditor::mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) {
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

bool GUIEditor::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboard_event(key, scancode, action, modifiers))
        return true;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        set_visible(false);
        return true;
    }
    return false;
}

void GUIEditor::draw(NVGcontext *ctx) {
    Screen::draw(ctx);
}

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
