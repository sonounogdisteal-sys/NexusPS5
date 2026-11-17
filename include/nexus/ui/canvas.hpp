#pragma once

#include <cstdint>
#include <string_view>

namespace nexus::ui {

struct Rect {
    int x;
    int y;
    int width;
    int height;
};

struct Color {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha = 255;
};

class Canvas {
public:
    virtual ~Canvas() = default;

    virtual void fill_rect(Rect rect, Color color) = 0;
    virtual void stroke_rect(Rect rect, Color color, int thickness = 1) = 0;
    virtual void draw_text(std::string_view text, int x, int y, int size, Color color) = 0;
    virtual void draw_line(int x1, int y1, int x2, int y2, Color color) = 0;
};

} // namespace nexus::ui
