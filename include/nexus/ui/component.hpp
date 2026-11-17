#pragma once

#include "nexus/ui/canvas.hpp"
#include "nexus/ui/focus_manager.hpp"

#include <string>
#include <vector>

namespace nexus::ui {

struct Component {
    std::string id;
    std::string label;
    Rect bounds;
    Color background{0, 0, 0, 0};
    Color foreground{235, 239, 248, 255};
    int font_size = 20;
    bool focusable = false;
    bool filled = false;
    bool accent = false;

    void render(Canvas& canvas, bool focused) const;
};

class Scene {
public:
    void set_components(std::vector<Component> components);
    void render(Canvas& canvas) const;

    [[nodiscard]] bool move(Direction direction);
    [[nodiscard]] bool focus_at(int x, int y);
    [[nodiscard]] std::string_view focused_id() const;
    [[nodiscard]] bool focus(std::string_view id);

private:
    std::vector<Component> components_;
    FocusManager focus_manager_;
};

} // namespace nexus::ui
