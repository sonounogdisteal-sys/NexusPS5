#include "nexus/ui/component.hpp"

#include <utility>

namespace nexus::ui {

void Component::render(Canvas& canvas, bool focused) const {
    if (filled) {
        canvas.fill_rect(bounds, background);
    }
    if (focused) {
        canvas.stroke_rect(bounds, accent ? Color{255, 255, 255, 255} : Color{77, 157, 255, 255}, 3);
    } else if (accent) {
        canvas.stroke_rect(bounds, Color{90, 156, 255, 220}, 1);
    }
    if (!label.empty()) {
        canvas.draw_text(label, bounds.x + 14, bounds.y + 10, font_size, foreground);
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

void Scene::render(Canvas& canvas) const {
    const std::string_view current_focus = focus_manager_.focused_id();
    for (const auto& component : components_) {
        component.render(canvas, component.id == current_focus);
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
