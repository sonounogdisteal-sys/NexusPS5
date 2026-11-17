#include "nexus/platform/sdl_canvas.hpp"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace nexus::platform {
namespace {

std::string sdl_error(std::string_view operation) {
    return std::string(operation) + ": " + SDL_GetError();
}

} // namespace

SdlCanvas::~SdlCanvas() {
    release();
}

bool SdlCanvas::initialize(
    std::string_view title,
    int width,
    int height,
    bool hidden,
    bool software_renderer,
    std::string& error
) {
    if (width <= 0 || height <= 0) {
        error = "Window dimensions must be positive.";
        return false;
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        error = sdl_error("SDL initialization failed");
        return false;
    }
    sdl_initialized_ = true;

    if (TTF_Init() != 0) {
        error = std::string("SDL_ttf initialization failed: ") + TTF_GetError();
        release();
        return false;
    }
    ttf_initialized_ = true;

    if (!load_font_path(error)) {
        release();
        return false;
    }

    const Uint32 flags = SDL_WINDOW_RESIZABLE | (hidden ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN);
    const std::string window_title(title);
    window_ = SDL_CreateWindow(
        window_title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        flags
    );
    if (window_ == nullptr) {
        error = sdl_error("Window creation failed");
        release();
        return false;
    }

    if (software_renderer) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    } else {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
        if (renderer_ == nullptr) {
            const std::string accelerated_error = SDL_GetError();
            renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
            if (renderer_ == nullptr) {
                error = "Renderer creation failed (accelerated: " + accelerated_error +
                    "; software: " + SDL_GetError() + ").";
                release();
                return false;
            }
            error = "Accelerated SDL renderer unavailable; using software renderer: " +
                accelerated_error;
        }
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if (SDL_RenderSetLogicalSize(renderer_, 1280, 720) != 0) {
        error = sdl_error("Logical render size setup failed");
        release();
        return false;
    }
    return true;
}

void SdlCanvas::present() {
    if (renderer_ == nullptr) {
        last_error_ = "Cannot present without an initialized renderer.";
        return;
    }
    SDL_RenderPresent(renderer_);
}

bool SdlCanvas::save_screenshot(const std::string& path, std::string& error) {
    if (renderer_ == nullptr) {
        error = "Cannot capture a screenshot without an initialized renderer.";
        return false;
    }

    int width = 0;
    int height = 0;
    if (SDL_GetRendererOutputSize(renderer_, &width, &height) != 0) {
        error = sdl_error("Renderer size query failed");
        return false;
    }
    if (width <= 0 || height <= 0) {
        error = "Cannot capture a screenshot from an empty renderer.";
        return false;
    }
    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(width) * height);
    if (SDL_RenderReadPixels(
            renderer_,
            nullptr,
            SDL_PIXELFORMAT_ARGB8888,
            pixels.data(),
            width * static_cast<int>(sizeof(std::uint32_t))
        ) != 0) {
        error = sdl_error("Screenshot readback failed");
        return false;
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        pixels.data(),
        width,
        height,
        32,
        width * static_cast<int>(sizeof(std::uint32_t)),
        SDL_PIXELFORMAT_ARGB8888
    );
    if (surface == nullptr) {
        error = sdl_error("Screenshot surface creation failed");
        return false;
    }
    const int save_result = SDL_SaveBMP(surface, path.c_str());
    if (save_result != 0) {
        error = sdl_error("Screenshot save failed");
    }
    SDL_FreeSurface(surface);
    return save_result == 0;
}

SDL_Window* SdlCanvas::window() const {
    return window_;
}

std::string_view SdlCanvas::last_error() const {
    return last_error_;
}

void SdlCanvas::window_to_logical(int& x, int& y) const {
    if (renderer_ != nullptr) {
        float logical_x = 0.0F;
        float logical_y = 0.0F;
        SDL_RenderWindowToLogical(renderer_, x, y, &logical_x, &logical_y);
        x = static_cast<int>(logical_x);
        y = static_cast<int>(logical_y);
    }
}

void SdlCanvas::fill_rect(ui::Rect rect, ui::Color color) {
    if (renderer_ == nullptr) {
        last_error_ = "Cannot draw without an initialized renderer.";
        return;
    }
    set_color(color);
    const SDL_Rect target{rect.x, rect.y, rect.width, rect.height};
    if (SDL_RenderFillRect(renderer_, &target) != 0) {
        last_error_ = sdl_error("Rectangle fill failed");
    }
}

void SdlCanvas::stroke_rect(ui::Rect rect, ui::Color color, int thickness) {
    if (renderer_ == nullptr) {
        last_error_ = "Cannot draw without an initialized renderer.";
        return;
    }
    set_color(color);
    for (int offset = 0; offset < thickness; ++offset) {
        const SDL_Rect target{
            rect.x + offset,
            rect.y + offset,
            rect.width - offset * 2,
            rect.height - offset * 2,
        };
        if (target.w > 0 && target.h > 0 && SDL_RenderDrawRect(renderer_, &target) != 0) {
            last_error_ = sdl_error("Rectangle outline failed");
            return;
        }
    }
}

void SdlCanvas::draw_text(std::string_view text, int x, int y, int size, ui::Color color) {
    if (renderer_ == nullptr || text.empty()) {
        return;
    }
    TTF_Font* font = font_for_size(size);
    if (font == nullptr) {
        return;
    }

    const std::string owned_text(text);
    SDL_Surface* surface = TTF_RenderUTF8_Blended(
        font,
        owned_text.c_str(),
        SDL_Color{color.red, color.green, color.blue, color.alpha}
    );
    if (surface == nullptr) {
        last_error_ = std::string("Text rasterization failed: ") + TTF_GetError();
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture == nullptr) {
        last_error_ = sdl_error("Text texture creation failed");
        SDL_FreeSurface(surface);
        return;
    }
    const SDL_Rect target{x, y, surface->w, surface->h};
    if (SDL_RenderCopy(renderer_, texture, nullptr, &target) != 0) {
        last_error_ = sdl_error("Text draw failed");
    }
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void SdlCanvas::draw_line(int x1, int y1, int x2, int y2, ui::Color color) {
    if (renderer_ == nullptr) {
        last_error_ = "Cannot draw without an initialized renderer.";
        return;
    }
    set_color(color);
    if (SDL_RenderDrawLine(renderer_, x1, y1, x2, y2) != 0) {
        last_error_ = sdl_error("Line draw failed");
    }
}

bool SdlCanvas::load_font_path(std::string& error) {
    if (const char* configured = std::getenv("NEXUS_FONT_PATH"); configured != nullptr) {
        if (!std::filesystem::is_regular_file(configured)) {
            error = "NEXUS_FONT_PATH does not name a font file: " + std::string(configured);
            return false;
        }
        font_path_ = configured;
        return true;
    }

    constexpr std::array candidates{
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "C:/Windows/Fonts/arial.ttf",
    };
    for (const char* candidate : candidates) {
        if (std::filesystem::is_regular_file(candidate)) {
            font_path_ = candidate;
            return true;
        }
    }
    error =
        "No supported system font found. Install DejaVu Sans or set NEXUS_FONT_PATH.";
    return false;
}

TTF_Font* SdlCanvas::font_for_size(int size) {
    const auto found = fonts_.find(size);
    if (found != fonts_.end()) {
        return found->second;
    }
    TTF_Font* font = TTF_OpenFont(font_path_.c_str(), size);
    if (font == nullptr) {
        last_error_ = std::string("Font load failed: ") + TTF_GetError();
        return nullptr;
    }
    fonts_.emplace(size, font);
    return font;
}

void SdlCanvas::set_color(ui::Color color) {
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, color.red, color.green, color.blue, color.alpha);
}

void SdlCanvas::release() {
    for (const auto& [size, font] : fonts_) {
        (void)size;
        TTF_CloseFont(font);
    }
    fonts_.clear();
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    if (ttf_initialized_) {
        TTF_Quit();
        ttf_initialized_ = false;
    }
    if (sdl_initialized_) {
        SDL_Quit();
        sdl_initialized_ = false;
    }
}

} // namespace nexus::platform
