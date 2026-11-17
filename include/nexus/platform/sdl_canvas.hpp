#pragma once

#include "nexus/ui/canvas.hpp"

#include <SDL.h>
#include <SDL_ttf.h>

#include <string>
#include <string_view>
#include <unordered_map>

namespace nexus::platform {

class SdlCanvas final : public ui::Canvas {
public:
    SdlCanvas() = default;
    ~SdlCanvas() override;

    SdlCanvas(const SdlCanvas&) = delete;
    SdlCanvas& operator=(const SdlCanvas&) = delete;

    [[nodiscard]] bool initialize(
        std::string_view title,
        int width,
        int height,
        bool hidden,
        bool software_renderer,
        std::string& error
    );
    void present();
    [[nodiscard]] bool save_screenshot(const std::string& path, std::string& error);
    [[nodiscard]] SDL_Window* window() const;
    [[nodiscard]] std::string_view last_error() const;
    void window_to_logical(int& x, int& y) const;

    void fill_rect(ui::Rect rect, ui::Color color) override;
    void stroke_rect(ui::Rect rect, ui::Color color, int thickness = 1) override;
    void draw_text(std::string_view text, int x, int y, int size, ui::Color color) override;
    void draw_line(int x1, int y1, int x2, int y2, ui::Color color) override;

private:
    [[nodiscard]] bool load_font_path(std::string& error);
    [[nodiscard]] TTF_Font* font_for_size(int size);
    void set_color(ui::Color color);
    void release();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    std::string font_path_;
    std::unordered_map<int, TTF_Font*> fonts_;
    std::string last_error_;
    bool sdl_initialized_ = false;
    bool ttf_initialized_ = false;
};

} // namespace nexus::platform
