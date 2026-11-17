#include "nexus/ui/component.hpp"

#include <utility>

#include <cmath>

namespace nexus::ui {

void Component::render(Canvas& canvas, bool focused, int vertical_offset) const {
    Rect visual_bounds = bounds;
    const float scale = focused ? 1.04F : 1.0F;
    visual_bounds.width = static_cast<int>(std::lround(static_cast<float>(bounds.width) * scale));
    visual_bounds.height = static_cast<int>(std::lround(static_cast<float>(bounds.height) * scale));
    visual_bounds.x -= (visual_bounds.width - bounds.width) / 2;
    visual_bounds.y += vertical_offset - (visual_bounds.height - bounds.height) / 2;
    if (filled) {
        canvas.fill_rect(visual_bounds, background);
    }
    if (focused) {
        canvas.stroke_rect(visual_bounds, accent ? Color{255, 255, 255, 255} : Color{77, 157, 255, 255}, 3);
    } else if (accent) {
        canvas.stroke_rect(visual_bounds, Color{90, 156, 255, 220}, 1);
    }
    if (!label.empty()) {
        canvas.draw_text(
            label,
            visual_bounds.x + 14,
            visual_bounds.y + 10,
            static_cast<int>(std::lround(static_cast<float>(font_size) * scale)),
            foreground
        );
    }
}

void Scene::set_components(std::vector<Component> components) {
    std::vector<FocusTarget> targets;
    targets.reserve(components.size());
    for (const auto& component : components) {
        if (component.focusable) {
            const int column = component.bounds.x / 10;
            const int row = component.bounds.y / 10;
            targets.push_back({component.id, column, row, component.bounds});
        }
    }
    focus_manager_.set_targets(std::move(targets));
    components_ = std::move(components);
}

void Scene::render(Canvas& canvas, int vertical_offset) const {
    const std::string_view current_focus = focus_manager_.focused_id();
    for (const auto& component : components_) {
        component.render(canvas, component.id == current_focus, vertical_offset);
    }
}

bool Scene::move(Direction direction) {
    return focus_manager_.move(direction);
}

bool Scene::focus_at(int x, int y) {
    return focus_manager_.focus_at(x, y);
}

std::string_view Scene::focused_id() const {
    return focus_manager_.focused_id();
}

bool Scene::focus(std::string_view id) {
    return focus_manager_.focus(id);
}

} // namespace nexus::ui
