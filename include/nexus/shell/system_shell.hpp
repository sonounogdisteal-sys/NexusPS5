#pragma once

#include "nexus/core/config.hpp"
#include "nexus/shell/game_hub.hpp"
#include "nexus/input/action.hpp"
#include "nexus/shell/control_center.hpp"
#include "nexus/shell/settings_screen.hpp"
#include "nexus/shell/welcome_hub.hpp"
#include "nexus/ui/animation.hpp"
#include "nexus/ui/component.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace nexus::shell {

class SystemShell {
public:
    struct WindowSettings {
        int width;
        int height;
        bool fullscreen;
    };

    explicit SystemShell(
        core::Config config = {},
        std::filesystem::path config_path = {},
        WelcomeHub welcome_hub = WelcomeHub{}
    );

    void render(ui::Canvas& canvas) const;
    void update(float delta_seconds);
    void handle(input::Action action);
    void mouse_move(int x, int y);
    void mouse_click(int x, int y);

    [[nodiscard]] std::optional<WindowSettings> take_window_update();
    void report_system_error(std::string message);
    [[nodiscard]] bool control_center_open() const;
    [[nodiscard]] std::string_view focused_id() const;
    [[nodiscard]] const std::string& status_message() const;

private:
    struct SampleApp {
        std::string title;
        std::string subtitle;
        ui::Color color;
    };

    enum class View {
        home,
        game_library,
        game_hub,
        settings,
        welcome_hub,
    };

    void rebuild_home_scene();
    void render_wallpaper(ui::Canvas& canvas) const;
    void render_game_library(ui::Canvas& canvas) const;
    void render_selected_entry(ui::Canvas& canvas) const;
    void activate_focused_component();
    void activate_library_component();
    void move_focus(ui::Direction direction);
    void rebuild_library_scene();
    void select_app(std::size_t index);
    void set_home_theme();
    [[nodiscard]] const SampleApp& selected_app() const;

    core::Config config_;
    std::filesystem::path config_path_;
    SettingsScreen settings_screen_;
    WelcomeHub welcome_hub_;
    GameHubScreen game_hub_;
    ui::Scene home_scene_;
    ui::Scene library_scene_;
    ControlCenterOverlay control_center_;
    ui::ColorAnimation background_animation_;
    std::vector<SampleApp> games_;
    std::vector<SampleApp> media_;
    std::size_t selected_app_index_ = 0;
    View view_ = View::home;
    bool showing_media_ = false;
    std::string status_message_ =
        "Local shell prototype - guest applications are not executable.";
};

} // namespace nexus::shell
