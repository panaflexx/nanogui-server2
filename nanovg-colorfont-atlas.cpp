#include <nanogui/nanogui.h>
#include <nanogui/opengl.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include "fontstash.h"

using namespace nanogui;



class AtlasScreen : public Screen {
public:
    AtlasScreen() : Screen(Vector2i(800, 600), "NanoGUI Atlas Texture Example") {
        // Initialize NanoVG is handled internally by NanoGUI Screen

        // Create a font with NanoVG's context
        int fontId = nvgCreateFont(m_nvg_context, "colorfont", "resources/NotoColorEmoji.ttf");
        if (fontId == -1) {
            throw std::runtime_error("Failed to load font");
        }

        // Show the screen
        perform_layout();
    }

    virtual void draw_contents() override {
        Screen::draw_contents();

		const char *text = "🎉🐺";
		float x = 0;
		float y = 100;
		float bounds[4] = {0};

		nvgFontSize(m_nvg_context, 48.0f);
		nvgFontFace(m_nvg_context, "colorfont");
		printf("draw: nvgText font=coloremoji size=32.0f, text=[%s]\n");
		nvgText(m_nvg_context, x, y, text, NULL);

		// Calculate text bounds
		//nvgFontFace(m_nvg_context, "colorfont");
		nvgTextBounds(m_nvg_context, x, y, text, NULL, bounds);
		printf("color bounds = [%.1f %.1f %.1f %.1f]\n", bounds[0], bounds[1], bounds[2], bounds[3]);

		y += bounds[3]-bounds[1];

		// Draw colorfont emoji bounding box
		nvgBeginPath(m_nvg_context);
		nvgStrokeWidth(m_nvg_context, 1.0f);
		nvgStrokeColor(m_nvg_context, nvgRGBA(255, 255, 0, 255)); // Yellow color
		nvgRect(m_nvg_context, bounds[0], bounds[1], bounds[2] - bounds[0], bounds[3] - bounds[1]);
		nvgStroke(m_nvg_context);

		printf("draw: nvgText font=sans size=32.0f, text=[X hits the spot]\n");
		nvgFontFace(m_nvg_context, "mono");
		nvgText(m_nvg_context, x, y, "X hits the snot", NULL);
		nvgTextBounds(m_nvg_context, x, y, "X hits the snot", NULL, bounds);
		printf("grayscale bounds = [%.1f %.1f %.1f %.1f]\n", bounds[0], bounds[1], bounds[2], bounds[3]);

		// Draw sans bounding box
		nvgBeginPath(m_nvg_context);
		nvgStrokeWidth(m_nvg_context, 1.0f);
		nvgStrokeColor(m_nvg_context, nvgRGBA(255, 0, 0, 255)); // Red color
		nvgRect(m_nvg_context, bounds[0], bounds[1], bounds[2] - bounds[0], bounds[3] - bounds[1]);
		nvgStroke(m_nvg_context);

		// Draw the text
		nvgFillColor(m_nvg_context, nvgRGBA(0, 0, 0, 255)); // Black color

		int texWidth=0, texHeight=0;
        const char* textureData = nvgFontTexture( m_nvg_context, &texWidth, &texHeight);
		//printf("Got texture %p width=%d height=%d\n", textureData, texWidth, texHeight);

        // Draw the atlas texture full-screen or at bottom right
        int winWidth = width(), winHeight = height();

        float drawX = winWidth - texWidth - 20;
        float drawY = winHeight - texHeight - 20;
        float drawW = (float)texWidth;
        float drawH = (float)texHeight;

		int texId = nvgCreateImageRGBA(m_nvg_context, texWidth, texHeight, 0,
		                           (const unsigned char*)textureData);

        // Draw the atlas as an image pattern
        NVGpaint imgPaint = nvgImagePattern(m_nvg_context,
            drawX, drawY, drawW, drawH,
            0.0f, texId, 1.0f);

        nvgBeginPath(m_nvg_context);
        nvgRect(m_nvg_context, drawX, drawY, drawW, drawH);
        nvgFillPaint(m_nvg_context, imgPaint);
        nvgFill(m_nvg_context);

    }
};

int main(int argc, char **argv) {
    try {
        nanogui::init();

        {
            AtlasScreen screen;
            screen.set_visible(true);
            screen.perform_layout();
            nanogui::mainloop();
        }

        nanogui::shutdown();
    } catch (const std::runtime_error &e) {
        printf("Error: %s\n", e.what() );
        return -1;
    }

    return 0;
}

