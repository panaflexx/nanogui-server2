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

#include <iostream>
#include <memory>

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
        if ( (editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
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
        if ( (editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
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
        if ( (editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
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
        if ( (editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
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
        if ( (editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
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
        if ( (editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
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
        if ( (editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
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
        if ( (editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
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
        if ( (editor && editor->selected_widget == this) || !TestModeManager::getInstance()->isTestModeEnabled()) {
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
    properties_pane->set_layout(new GroupLayout());
    update_properties();

    // Canvas window (empty for placing widgets)
    canvas_win = new Window(this, "Canvas");
    canvas_win->set_position(Vector2i(280, 15));
    canvas_win->set_size(Vector2i(700, 700));
    canvas_win->set_layout(nullptr); // Absolute positioning

    perform_layout();
}

void GUIEditor::update_properties() {
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
        (void)c;
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

    if (!TestModeManager::getInstance()->isTestModeEnabled() &&
				button == GLFW_MOUSE_BUTTON_1) 
	{
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
