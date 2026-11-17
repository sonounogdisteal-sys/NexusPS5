#pragma once

#include "nexus/input/action.hpp"
#include "nexus/shell/control_center.hpp"
#include "nexus/ui/component.hpp"

#include <string>
#include <vector>

namespace nexus::shell {

class SystemShell {
public:
    SystemShell();

    void render(ui::Canvas& canvas) const;
    void handle(input::Action action);
    void mouse_move(int x, int y);
    void mouse_click(int x, int y);

    [[nodiscard]] bool control_center_open() const;
    [[nodiscard]] std::string_view focused_id() const;
    [[nodiscard]] const std::string& status_message() const;

private:
    struct SampleApp {
        std::string title;
        std::string subtitle;
        ui::Color color;
    };

    void rebuild_home_scene();
    void render_wallpaper(ui::Canvas& canvas) const;
    void render_welcome_hub(ui::Canvas& canvas) const;
    void render_selected_entry(ui::Canvas& canvas) const;
    void activate_focused_component();
    void move_focus(ui::Direction direction);
    [[nodiscard]] const SampleApp& selected_app() const;

    ui::Scene home_scene_;
    ControlCenterOverlay control_center_;
    std::vector<SampleApp> games_;
    std::vector<SampleApp> media_;
    std::size_t selected_app_index_ = 0;
    bool showing_media_ = false;
    std::string status_message_ =
        "Local shell prototype - guest applications are not executable.";
};

} // namespace nexus::shell
