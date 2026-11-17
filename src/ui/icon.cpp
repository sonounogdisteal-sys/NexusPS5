#include "nexus/ui/icon.hpp"

namespace nexus::ui {

Icon::Icon(IconKind kind) : kind_(kind) {}

void Icon::render(Canvas& canvas, Rect bounds, Color color) const {
    const int left = bounds.x;
    const int top = bounds.y;
    const int right = left + bounds.width - 1;
    const int bottom = top + bounds.height - 1;
    const int center_x = left + bounds.width / 2;
    const int center_y = top + bounds.height / 2;

    switch (kind_) {
    case IconKind::home:
        canvas.draw_line(left + 1, center_y, center_x, top + 1, color);
        canvas.draw_line(center_x, top + 1, right - 1, center_y, color);
        canvas.draw_line(left + 4, center_y - 1, left + 4, bottom - 1, color);
        canvas.draw_line(right - 4, center_y - 1, right - 4, bottom - 1, color);
        canvas.draw_line(left + 4, bottom - 1, right - 4, bottom - 1, color);
        canvas.draw_line(center_x, bottom - 1, center_x, center_y + 3, color);
        break;
    case IconKind::library:
        canvas.stroke_rect({left + 1, top + 2, bounds.width - 5, bounds.height - 4}, color, 1);
        canvas.stroke_rect({left + 5, top + 1, bounds.width - 5, bounds.height - 3}, color, 1);
        canvas.draw_line(left + 8, top + 5, right - 2, top + 5, color);
        canvas.draw_line(left + 8, top + 8, right - 2, top + 8, color);
        break;
    case IconKind::profile:
        canvas.draw_line(center_x - 2, top + 2, center_x + 2, top + 2, color);
        canvas.draw_line(center_x - 4, top + 4, center_x - 4, top + 7, color);
        canvas.draw_line(center_x + 4, top + 4, center_x + 4, top + 7, color);
        canvas.draw_line(center_x - 2, top + 9, center_x + 2, top + 9, color);
        canvas.draw_line(left + 2, bottom - 2, left + 4, bottom - 6, color);
        canvas.draw_line(left + 4, bottom - 6, center_x, bottom - 8, color);
        canvas.draw_line(center_x, bottom - 8, right - 4, bottom - 6, color);
        canvas.draw_line(right - 4, bottom - 6, right - 2, bottom - 2, color);
        break;
    case IconKind::settings:
        canvas.draw_line(center_x, top + 1, center_x, top + 4, color);
        canvas.draw_line(center_x, bottom - 4, center_x, bottom - 1, color);
        canvas.draw_line(left + 1, center_y, left + 4, center_y, color);
        canvas.draw_line(right - 4, center_y, right - 1, center_y, color);
        canvas.draw_line(left + 3, top + 3, left + 5, top + 5, color);
        canvas.draw_line(right - 5, bottom - 5, right - 3, bottom - 3, color);
        canvas.draw_line(right - 3, top + 3, right - 5, top + 5, color);
        canvas.draw_line(left + 5, bottom - 5, left + 3, bottom - 3, color);
        canvas.stroke_rect({center_x - 2, center_y - 2, 5, 5}, color, 1);
        break;
    }
}

} // namespace nexus::ui
