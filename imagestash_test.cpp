#include <nanogui/nanogui.h>
#include <nanogui/opengl.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <pthread.h>

extern "C" {
#include "nanovg.h"
#include "imagestash.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize2.h"
}

using namespace nanogui;

class ImageStashScreen : public Screen {
public:
    ImageStashScreen(const std::vector<std::string>& imagePaths)
        : Screen(Vector2i(800, 600), "NanoGUI ImageStash Test"), m_imagePaths(imagePaths) {
        // Initialize NanoVG context (handled by NanoGUI)
        if (!m_nvg_context) {
            throw std::runtime_error("NanoVG context not initialized");
        }

        // Load a font for text rendering
        int fontId = nvgCreateFont(m_nvg_context, "sans", "Roboto-Regular.ttf");
        if (fontId == -1) {
            std::cerr << "Could not load font 'Roboto-Regular.ttf'" << std::endl;
        }

        // Initialize imagestash
        IMGSparams params = {512, 512, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        m_stash = imgsCreateInternal(&params);
        if (!m_stash) {
            throw std::runtime_error("Failed to create imagestash");
        }

        // Create NanoVG image for the atlas
        imgsGetAtlasSize(m_stash, &m_atlasWidth, &m_atlasHeight);
        m_atlasImage = nvgCreateImageRGBA(m_nvg_context, m_atlasWidth, m_atlasHeight, 0,
                                          imgsGetTextureData(m_stash, nullptr, nullptr));
        if (m_atlasImage == 0) {
            imgsDeleteInternal(m_stash);
            throw std::runtime_error("Failed to create NanoVG image for atlas");
        }

        // Initialize mutex
        pthread_mutex_init(&m_highResMutex, nullptr);

        // Start thread to load images progressively
        pthread_create(&m_loadThread, nullptr, loadImages, this);
        m_threadRunning = true;

        perform_layout();
    }

    ~ImageStashScreen() {
        // Cancel and join any running thread
        if (m_threadRunning) {
            pthread_cancel(m_loadThread);
            pthread_join(m_loadThread, nullptr);
            m_threadRunning = false;
        }

        // Clean up pending images
        pthread_mutex_lock(&m_highResMutex);
        for (auto& pending : m_pendingImages) {
            if (pending.pixels) {
                stbi_image_free(pending.pixels);
            }
        }
        if (m_pendingHighResPixels) {
            stbi_image_free(m_pendingHighResPixels);
        }
        if (m_fullscreenHighResImg) {
            stbi_image_free(m_fullscreenHighResImg->pixels);
            free(m_fullscreenHighResImg);
        }
        if (m_highResImage != 0) {
            nvgDeleteImage(m_nvg_context, m_highResImage);
        }
        pthread_mutex_unlock(&m_highResMutex);
        pthread_mutex_destroy(&m_highResMutex);

        // Clean up filtered image
        if (m_filteredImg) {
            imgsDeleteImage(m_filteredImg);
        }

        // Clean up stash and atlas image
        if (m_stash) {
            imgsDeleteInternal(m_stash);
        }
        if (m_atlasImage != 0) {
            nvgDeleteImage(m_nvg_context, m_atlasImage);
        }
    }

    struct PendingImage {
        std::string name;
        std::string path;
        unsigned char* pixels = nullptr;
        int width = 0;
        int height = 0;
    };

    struct LoadThreadData {
        ImageStashScreen* screen;
        std::string path;
    };

    static void* loadImagesOld(void* arg) {
        ImageStashScreen* screen = (ImageStashScreen*)arg;
        printf("loadImages start\n");

        // Enable cancellation
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);

        for (const auto& path : screen->m_imagePaths) {
            std::string name = path.substr(path.find_last_of("/\\") + 1);
            int w, h, n;
            unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &n, 4); // Force RGBA
            pthread_testcancel(); // Cancellation point
            if (!pixels) {
                std::cerr << "Failed to load image: " << path << std::endl;
                continue;
            }

            printf("loadImages loaded: %s, %dx%d\n", name.c_str(), w, h);

            pthread_mutex_lock(&screen->m_highResMutex);
            if (screen->m_threadRunning) {
                screen->m_pendingImages.push_back({name, path, pixels, w, h});
                screen->m_redraw = true;
                glfwPostEmptyEvent();
            } else {
                stbi_image_free(pixels);
            }
            pthread_mutex_unlock(&screen->m_highResMutex);
        }

        pthread_mutex_lock(&screen->m_highResMutex);
        screen->m_threadRunning = false;
        pthread_mutex_unlock(&screen->m_highResMutex);
        printf("loadImages completed\n");
        return nullptr;
    }

	static void* loadImages(void* arg) {
		ImageStashScreen* screen = (ImageStashScreen*)arg;
		printf("loadImages start\n");

		// Enable cancellation
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);

		const int maxWidth = 128;
		const int maxHeight = 128;

		for (const auto& path : screen->m_imagePaths) {
			std::string name = path.substr(path.find_last_of("/\\") + 1);
			int w, h, n;
			unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &n, 4); // Force RGBA
			pthread_testcancel(); // Cancellation point
			if (!pixels) {
				std::cerr << "Failed to load image: " << path << std::endl;
				continue;
			}

			// Resize if needed
			unsigned char* data = pixels;
			if (w > maxWidth || h > maxHeight) {
				float scale = fminf((float)maxWidth / w, (float)maxHeight / h);
				int nw = (int)(w * scale);
				int nh = (int)(h * scale);
				unsigned char* resized = (unsigned char*)malloc(nw * nh * 4);
				if (!resized) {
					stbi_image_free(data);
					std::cerr << "Failed to allocate memory for resized image: " << name << std::endl;
					continue;
				}

				if (!stbir_resize_uint8_linear(data, w, h, 0, resized, nw, nh, 0, STBIR_RGBA)) {
					free(resized);
					stbi_image_free(data);
					std::cerr << "Failed to resize image: " << name << std::endl;
					continue;
				}
				stbi_image_free(data);
				data = resized;
				w = nw;
				h = nh;
			}

			printf("loadImages loaded: %s, %dx%d\n", name.c_str(), w, h);

			pthread_mutex_lock(&screen->m_highResMutex);
			if (screen->m_threadRunning) {
				screen->m_pendingImages.push_back({name, path, data, w, h});
				screen->m_redraw = true;
				glfwPostEmptyEvent();
			} else {
				free(data);
			}
			pthread_mutex_unlock(&screen->m_highResMutex);
		}

		pthread_mutex_lock(&screen->m_highResMutex);
		screen->m_threadRunning = false;
		pthread_mutex_unlock(&screen->m_highResMutex);
		printf("loadImages completed\n");
		return nullptr;
	}

    static void* loadHighResImage(void* arg) {
        printf("loadHighResImage start\n");
        LoadThreadData* data = (LoadThreadData*)arg;
        ImageStashScreen* screen = data->screen;
        const std::string& path = data->path;

        // Enable cancellation
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);

        // Load high-resolution image
        int w, h, n;
        unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &n, 4); // Force RGBA
        pthread_testcancel(); // Cancellation point
        printf("loadHighResImage loaded: %dx%d\n", w, h);

        if (!pixels) {
            std::cerr << "Failed to load high-res image: " << path << std::endl;
            free(data);
            return nullptr;
        }

        // Store pixel data for main thread
        pthread_mutex_lock(&screen->m_highResMutex);
        if (screen->m_threadRunningHighRes) {
            if (screen->m_pendingHighResPixels) {
                stbi_image_free(screen->m_pendingHighResPixels);
            }
            screen->m_pendingHighResPixels = pixels;
            screen->m_pendingHighResWidth = w;
            screen->m_pendingHighResHeight = h;
            screen->m_redraw = true;
            glfwPostEmptyEvent();
        } else {
            stbi_image_free(pixels);
        }
        pthread_mutex_unlock(&screen->m_highResMutex);
        printf("loadHighResImage completed\n");

        free(data);
        return nullptr;
    }

    bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override {
        if (button == GLFW_MOUSE_BUTTON_LEFT && down) {
			if (m_fullscreenImage != -1) {
				// In fullscreen mode, exit fullscreen and cancel loading thread
				printf("Exit fullscreen mode\n");
				pthread_mutex_lock(&m_highResMutex);
				if (m_threadRunningHighRes) {
					pthread_cancel(m_loadHighResThread);
					pthread_join(m_loadHighResThread, nullptr);
					m_threadRunningHighRes = false;
				}
				if (m_pendingHighResPixels) {
					stbi_image_free(m_pendingHighResPixels);
					m_pendingHighResPixels = nullptr;
					m_pendingHighResWidth = 0;
					m_pendingHighResHeight = 0;
				}
				if (m_fullscreenHighResImg) {
					stbi_image_free(m_fullscreenHighResImg->pixels);
					free(m_fullscreenHighResImg);
					m_fullscreenHighResImg = nullptr;
				}
				if (m_highResImage != 0) {
					printf("Delete m_highResImage\n");
					nvgDeleteImage(m_nvg_context, m_highResImage);
					m_highResImage = 0;
				}
				m_threadRunningHighRes = false; // Ensure reset
				pthread_mutex_unlock(&m_highResMutex);

				m_animationStart = std::chrono::steady_clock::now();
				m_animationEnd = m_animationStart + std::chrono::milliseconds(400);
				m_isEnteringFullscreen = false;
				m_fullscreenImage = -1;
				m_redraw = true;
				glfwPostEmptyEvent();
				return true; // return so click doesn't go through to select new image
			}

            // In thumbnail mode, check if click is on an image
            float posx = 10;
            float posy = 50;
            float maxHeightInRow = 0;
            const float borderSize = 4.0f;
            const float textHeight = 20.0f;
            const float padding = 10.0f;

            for (size_t i = 0; i < m_images.size(); ++i) {
                const auto& img = m_images[i];
                if (img.atlasX < 0 || img.atlasY < 0) continue;

                if (posx + img.w + 2 * borderSize + padding > width() - 10) {
                    posx = 10;
                    posy += maxHeightInRow + textHeight + padding;
                    maxHeightInRow = 0;
                }

                maxHeightInRow = std::max(maxHeightInRow, (float)img.h + 2 * borderSize);

                // Check if click is within image bounds (including border)
                if (p.x() >= posx - borderSize && p.x() <= posx + img.w + borderSize &&
                    p.y() >= posy - borderSize && p.y() <= posy + img.h + borderSize) {
                    m_fullscreenImage = (int)i;
                    m_animationStart = std::chrono::steady_clock::now();
                    m_animationEnd = m_animationStart + std::chrono::milliseconds(400);
                    m_isEnteringFullscreen = true;

                    // Start thread to load high-res image
                    LoadThreadData* data = new LoadThreadData{this, img.path};
                    pthread_mutex_lock(&m_highResMutex);
                    m_threadRunningHighRes = true;
                    pthread_create(&m_loadHighResThread, nullptr, loadHighResImage, data);
                    pthread_mutex_unlock(&m_highResMutex);
                    m_redraw = true;
                    glfwPostEmptyEvent();
                    return true;
                }

                posx += img.w + 2 * borderSize + padding;
            }
        }
        return Screen::mouse_button_event(p, button, down, modifiers);
    }

    virtual void draw_contents() override {
        Screen::draw_contents();

        // Process pending images
        pthread_mutex_lock(&m_highResMutex);
        for (auto& pending : m_pendingImages) {
            if (imgsAddPixels(m_stash, pending.name.c_str(), pending.pixels, pending.width, pending.height, 0)) {
                TestImage img;
                img.name = pending.name;
                img.path = pending.path;
                IMGimage* tmp = imgsGet(m_stash, pending.name.c_str());
                if (tmp) {
                    img.w = tmp->width;
                    img.h = tmp->height;
                    img.atlasX = tmp->atlasX;
                    img.atlasY = tmp->atlasY;
                    imgsDeleteImage(tmp);
                } else {
                    img.w = pending.width;
                    img.h = pending.height;
                    img.atlasX = -1;
                    img.atlasY = -1;
                }
                m_images.push_back(img);
                m_redraw = true;
                glfwPostEmptyEvent();
            } else {
                std::cerr << "Failed to add image to stash: " << pending.name << std::endl;
            }
            stbi_image_free(pending.pixels);
        }
        m_pendingImages.clear();

        // Check for pending high-res image and create texture
		if (m_pendingHighResPixels) {
			printf("m_pendingHighResPixels active...\n");

			if (m_fullscreenHighResImg) {
				stbi_image_free(m_fullscreenHighResImg->pixels);
				free(m_fullscreenHighResImg);
			}
			if (m_highResImage != 0) {
				nvgDeleteImage(m_nvg_context, m_highResImage);
			}

			// Create IMGimage for tracking dimensions
			m_fullscreenHighResImg = (IMGimage*)malloc(sizeof(IMGimage));
			if (m_fullscreenHighResImg) {
				m_fullscreenHighResImg->ctx = m_stash;
				m_fullscreenHighResImg->atlasX = -1;
				m_fullscreenHighResImg->atlasY = -1;
				m_fullscreenHighResImg->width = m_pendingHighResWidth;
				m_fullscreenHighResImg->height = m_pendingHighResHeight;
				m_fullscreenHighResImg->pixels = m_pendingHighResPixels;
				m_fullscreenHighResImg->ownedPixels = 1;
				m_fullscreenHighResImg->dirty = 0;

				// Create NanoVG texture
				m_highResImage = nvgCreateImageRGBA(m_nvg_context, m_pendingHighResWidth, m_pendingHighResHeight, 0, m_pendingHighResPixels);
				if (m_highResImage == 0) {
					std::cerr << "Failed to create NanoVG image for high-res" << std::endl;
					stbi_image_free(m_pendingHighResPixels);
					free(m_fullscreenHighResImg);
					m_fullscreenHighResImg = nullptr;
				}
			} else {
				stbi_image_free(m_pendingHighResPixels);
			}
			m_pendingHighResPixels = nullptr;
			m_pendingHighResWidth = 0;
			m_pendingHighResHeight = 0;
			m_redraw = true;
			glfwPostEmptyEvent();
		}
        
        pthread_mutex_unlock(&m_highResMutex);

        // Filter the first image if available and not yet filtered
        if (!m_filteredImg && !m_images.empty()) {
            m_filteredImg = imgsGet(m_stash, m_images[0].name.c_str());
            if (m_filteredImg) {
                imgsFilterGreyscale(m_filteredImg);
                imgsFilterBlur(m_filteredImg, 20.0f);
                imgsFilterResize(m_filteredImg, 128, 128);
                m_redraw = true;
                glfwPostEmptyEvent();
            }
        }

        // Begin NanoVG frame
        nvgBeginFrame(m_nvg_context, (float)width(), (float)height(), 1.0f);

        // Check if atlas size has changed and recreate NanoVG image if needed
        int curWidth, curHeight;
        imgsGetAtlasSize(m_stash, &curWidth, &curHeight);
        if (curWidth != m_atlasWidth || curHeight != m_atlasHeight) {
            nvgDeleteImage(m_nvg_context, m_atlasImage);
            m_atlasImage = nvgCreateImageRGBA(m_nvg_context, curWidth, curHeight, 0,
                                              imgsGetTextureData(m_stash, nullptr, nullptr));
            m_atlasWidth = curWidth;
            m_atlasHeight = curHeight;
            m_redraw = true;
            glfwPostEmptyEvent();
        }

        // Update atlas texture if dirty
        int dirty[4];
        if (imgsValidateTexture(m_stash, dirty)) {
            const unsigned char* data = imgsGetTextureData(m_stash, nullptr, nullptr);
            nvgUpdateImage(m_nvg_context, m_atlasImage, data);
        }

        float itw = 1.0f / m_atlasWidth;
        float ith = 1.0f / m_atlasHeight;

        // Animation progress
        float t = 0.0f;
        if (m_fullscreenImage != -1) {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_animationEnd - m_animationStart).count();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_animationStart).count();
            t = std::min(std::max((float)elapsed / duration, 0.0f), 1.0f);
            t = t * t * (3.0f - 2.0f * t); // Ease-in-out
            if (!m_isEnteringFullscreen) {
                t = 1.0f - t;
                m_redraw = true;
            }
            if (now >= m_animationEnd) {
				if (!m_isEnteringFullscreen) {
					// Cancel pending fullscreen image
					pthread_mutex_lock(&m_highResMutex);
					if (m_threadRunningHighRes) {
						pthread_cancel(m_loadHighResThread);
						pthread_join(m_loadHighResThread, nullptr);
						m_threadRunningHighRes = false;
					}
					if (m_pendingHighResPixels) {
						stbi_image_free(m_pendingHighResPixels);
						m_pendingHighResPixels = nullptr;
						m_pendingHighResWidth = 0;
						m_pendingHighResHeight = 0;
					}
					if (m_fullscreenHighResImg) {
						stbi_image_free(m_fullscreenHighResImg->pixels);
						free(m_fullscreenHighResImg);
						m_fullscreenHighResImg = nullptr;
					}
					if (m_highResImage != 0) {
						nvgDeleteImage(m_nvg_context, m_highResImage);
						m_highResImage = 0;
					}
					m_threadRunningHighRes = false;
					m_fullscreenImage = -1; // Reset here to ensure state is cleared
					pthread_mutex_unlock(&m_highResMutex);
					m_redraw = true;
					glfwPostEmptyEvent();
					printf("Cancelled high res load\n");
				}
			}
        }

        // Draw background (fade to black in fullscreen)
        m_backgroundOpacity = t;
        nvgBeginPath(m_nvg_context);
        nvgRect(m_nvg_context, 0, 0, (float)width(), (float)height());
        nvgFillColor(m_nvg_context, nvgRGBA(0, 0, 0, (unsigned char)(m_backgroundOpacity * 255)));
        nvgFill(m_nvg_context);

        // Draw images or fullscreen image
        float posx = 10;
        float posy = 50;
        float maxHeightInRow = 0;
        const float borderSize = 4.0f;
        const float textHeight = 20.0f;
        const float padding = 10.0f;

        for (size_t i = 0; i < m_images.size(); ++i) {
            const auto& img = m_images[i];
            if (img.atlasX < 0 || img.atlasY < 0) continue;

            // Calculate thumbnail position
            if (posx + img.w + 2 * borderSize + padding > width() - 10) {
                posx = 10;
                posy += maxHeightInRow + textHeight + padding;
                maxHeightInRow = 0;
            }
            maxHeightInRow = std::max(maxHeightInRow, (float)img.h + 2 * borderSize);

            // Fullscreen position
            float full_w, full_h, full_x, full_y;
            bool useHighRes = false;

			if(m_highResImage > 0)
				printf("Have highResImage\n");

            pthread_mutex_lock(&m_highResMutex);
            if ((int)i == m_fullscreenImage && m_highResImage > 0 && t > 0.2f) {
                useHighRes = true;
                full_w = m_fullscreenHighResImg->width;
                full_h = m_fullscreenHighResImg->height;
                float scale = std::min((float)width() / full_w, (float)height() / full_h) * 0.9f;
                full_w *= scale;
                full_h *= scale;
                full_x = (width() - full_w) / 2.0f;
                full_y = (height() - full_h) / 2.0f;
            } else {
                float scale = std::min((float)width() / img.w, (float)height() / img.h) * 0.9f;
                full_w = img.w * scale;
                full_h = img.h * scale;
                full_x = (width() - full_w) / 2.0f;
                full_y = (height() - full_h) / 2.0f;
            }
            pthread_mutex_unlock(&m_highResMutex);

            // Interpolate position and size
            float draw_x = posx;
            float draw_y = posy;
            float draw_w = (float)img.w;
            float draw_h = (float)img.h;
            float alpha = 1.0f;

            if ((int)i == m_fullscreenImage) {
                draw_x = posx + t * (full_x - posx);
                draw_y = posy + t * (full_y - posy);
                draw_w = img.w + t * (full_w - img.w);
                draw_h = img.h + t * (full_h - img.h);
                alpha = 1.0f;
            } else if (m_fullscreenImage != -1) {
                alpha = 1.0f - t; // Fade out non-selected images
            }

            // Draw blue border
#if 0
            nvgBeginPath(m_nvg_context);
            nvgRect(m_nvg_context, draw_x - borderSize, draw_y - borderSize,
                    draw_w + 2 * borderSize, draw_h + 2 * borderSize);
            nvgFillColor(m_nvg_context, nvgRGBA(0, 0, 255, (unsigned char)(255 * alpha)));
            nvgFill(m_nvg_context);
#endif

            // Draw image
            nvgSave(m_nvg_context);
            nvgTranslate(m_nvg_context, draw_x, draw_y);

            if (useHighRes) {
                // Use high-res image
                NVGpaint imgPaint = nvgImagePattern(m_nvg_context, 0, 0, draw_w, draw_h, 0.0f, m_highResImage, alpha);
                nvgBeginPath(m_nvg_context);
                nvgRect(m_nvg_context, 0, 0, draw_w, draw_h);
                nvgFillPaint(m_nvg_context, imgPaint);
            } else {
                // Use atlas image
                float u0 = (img.atlasX + IMGS_PAD) * itw;
                float v0 = (img.atlasY + IMGS_PAD) * ith;
                float u1 = (img.atlasX + IMGS_PAD + img.w) * itw;
                float v1 = (img.atlasY + IMGS_PAD + img.h) * ith;
                nvgScale(m_nvg_context, draw_w / (u1 - u0) / m_atlasWidth, draw_h / (v1 - v0) / m_atlasHeight);
                nvgTranslate(m_nvg_context, -u0 * m_atlasWidth, -v0 * m_atlasHeight);
                NVGpaint imgPaint = nvgImagePattern(m_nvg_context, 0, 0, m_atlasWidth, m_atlasHeight, 0.0f, m_atlasImage, alpha);
                nvgBeginPath(m_nvg_context);
                nvgRect(m_nvg_context, u0 * m_atlasWidth, v0 * m_atlasHeight, img.w, img.h);
                nvgFillPaint(m_nvg_context, imgPaint);
            }

            nvgFill(m_nvg_context);
            nvgRestore(m_nvg_context);

            // Draw filename below the image (only in thumbnail mode)
            if ((int)i != m_fullscreenImage || t < 0.5f) {
                nvgFontSize(m_nvg_context, 14.0f);
                nvgFontFace(m_nvg_context, "sans");
                nvgFillColor(m_nvg_context, nvgRGBA(255, 255, 255, (unsigned char)(255 * alpha)));
                nvgTextAlign(m_nvg_context, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                nvgText(m_nvg_context, draw_x + draw_w / 2.0f, draw_y + draw_h + borderSize, img.name.c_str(), nullptr);
            }

            posx += img.w + 2 * borderSize + padding;
        }

        // Draw filtered image separately with a label
        if (m_filteredImg && m_fullscreenImage == -1) {
            if (posx + m_filteredImg->width + 2 * borderSize + padding > width() - 10) {
                posx = 10;
                posy += maxHeightInRow + textHeight + padding;
            }

            nvgBeginPath(m_nvg_context);
            nvgRect(m_nvg_context, posx - borderSize, posy - borderSize,
                    m_filteredImg->width + 2 * borderSize, m_filteredImg->height + 2 * borderSize);
            nvgFillColor(m_nvg_context, nvgRGBA(255, 0, 255, 255));
            nvgFill(m_nvg_context);

            float u0 = (m_filteredImg->atlasX + IMGS_PAD) * itw;
            float v0 = (m_filteredImg->atlasY + IMGS_PAD) * ith;
            float u1 = (m_filteredImg->atlasX + IMGS_PAD + m_filteredImg->width) * itw;
            float v1 = (m_filteredImg->atlasY + IMGS_PAD + m_filteredImg->height) * ith;

            nvgSave(m_nvg_context);
            nvgTranslate(m_nvg_context, posx, posy);
            nvgScale(m_nvg_context, (float)m_filteredImg->width / (u1 - u0) / m_atlasWidth,
                     (float)m_filteredImg->height / (v1 - v0) / m_atlasHeight);
            nvgTranslate(m_nvg_context, -u0 * m_atlasWidth, -v0 * m_atlasHeight);

            NVGpaint imgPaint = nvgImagePattern(m_nvg_context, 0, 0, m_atlasWidth, m_atlasHeight, 0.0f, m_atlasImage, 1.0f);
            nvgBeginPath(m_nvg_context);
            nvgRect(m_nvg_context, u0 * m_atlasWidth, v0 * m_atlasHeight, m_filteredImg->width, m_filteredImg->height);
            nvgFillPaint(m_nvg_context, imgPaint);
            nvgFill(m_nvg_context);

            nvgRestore(m_nvg_context);

            nvgFontSize(m_nvg_context, 14.0f);
            nvgFontFace(m_nvg_context, "sans");
            nvgFillColor(m_nvg_context, nvgRGBA(255, 255, 255, 255));
            nvgTextAlign(m_nvg_context, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            std::string filteredName = m_images[0].name + " (filtered)";
            nvgText(m_nvg_context, posx + m_filteredImg->width / 2.0f, posy + m_filteredImg->height + borderSize,
                    filteredName.c_str(), nullptr);
        }

        // Draw title (only in thumbnail mode)
        if (m_fullscreenImage == -1) {
            nvgFontSize(m_nvg_context, 18.0f);
            nvgFontFace(m_nvg_context, "sans");
            nvgFillColor(m_nvg_context, nvgRGBA(255, 255, 255, 128));
            nvgText(m_nvg_context, 100, 20, "Loaded Images:", nullptr);
        }

        nvgEndFrame(m_nvg_context);
    }

private:
    struct TestImage {
        std::string name;
        std::string path;
        int w, h;
        int atlasX, atlasY;
    };

    std::vector<TestImage> m_images;
    std::vector<std::string> m_imagePaths;
    std::vector<PendingImage> m_pendingImages;
    IMGcontext* m_stash = nullptr;
    IMGimage* m_filteredImg = nullptr;
    IMGimage* m_fullscreenHighResImg = nullptr;
    unsigned char* m_pendingHighResPixels = nullptr;
    int m_pendingHighResWidth = 0;
    int m_pendingHighResHeight = 0;
    int m_atlasImage = 0;
    int m_atlasWidth = 0;
    int m_atlasHeight = 0;
    int m_highResImage = 0;
    int m_fullscreenImage = -1;
    std::chrono::steady_clock::time_point m_animationStart;
    std::chrono::steady_clock::time_point m_animationEnd;
    bool m_isEnteringFullscreen = false;
    bool m_redraw = true;
    float m_backgroundOpacity = 0.0f;
    pthread_t m_loadThread;
    pthread_t m_loadHighResThread;
    bool m_threadRunning = false;
    bool m_threadRunningHighRes = false;
    pthread_mutex_t m_highResMutex;
};

int main(int argc, char** argv) {
    try {
        nanogui::init();

        // Collect image paths from command line
        std::vector<std::string> imagePaths;
        for (int i = 1; i < argc; ++i) {
            imagePaths.emplace_back(argv[i]);
        }

        // Create and show the screen
        ref<ImageStashScreen> screen = new ImageStashScreen(imagePaths);
        screen->set_visible(true);
        screen->perform_layout();
        nanogui::mainloop(1/30.f * 1000);

        nanogui::shutdown();
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
