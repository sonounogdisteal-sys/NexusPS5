#include "nexus/input/action.hpp"
#include "nexus/shell/control_center.hpp"
#include "nexus/shell/game_hub.hpp"
#include "nexus/shell/settings_screen.hpp"
#include "nexus/shell/system_shell.hpp"
#include "nexus/shell/welcome_hub.hpp"
#include "nexus/ui/animation.hpp"
#include "nexus/ui/canvas.hpp"
#include "nexus/ui/focus_manager.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void test_focus_navigation() {
    nexus::ui::FocusManager focus;
    focus.set_targets({
        {"left", 0, 0, {0, 0, 40, 40}},
        {"right", 1, 0, {50, 0, 40, 40}},
        {"below", 0, 1, {0, 50, 40, 40}},
    });
    require(focus.focused_id() == "left", "first focusable item should receive initial focus");
    require(focus.move(nexus::ui::Direction::right), "right navigation should find a neighbor");
    require(focus.focused_id() == "right", "right navigation should focus aligned neighbor");
    require(!focus.move(nexus::ui::Direction::right), "navigation should not wrap implicitly");
    require(focus.move(nexus::ui::Direction::down), "down navigation should find a neighbor");
    require(focus.focused_id() == "below", "vertical focus transition should be correct");
}

void test_focus_transition_and_pointer() {
    nexus::ui::FocusManager focus;
    focus.set_targets({
        {"one", 0, 0, {10, 10, 50, 50}},
        {"two", 1, 0, {70, 10, 50, 50}},
    });
    require(focus.focus("two"), "focus by id should succeed");
    require(focus.focus_at(20, 20), "pointer hit should focus a target");
    require(focus.focused_id() == "one", "pointer should transition focus to hit target");
    require(!focus.focus_at(200, 200), "pointer miss should preserve current focus");
    require(focus.focused_id() == "one", "pointer miss must not clear current focus");
}

void test_shell_navigation_and_control_center() {
    nexus::shell::SystemShell shell;
    require(!shell.control_center_open(), "Control Center should start closed");
    require(shell.focused_id() == "app.0", "Home should initially focus the first sample tile");
    shell.handle(nexus::input::Action::move_right);
    require(shell.focused_id() == "app.1", "horizontal input should move Home focus");
    shell.handle(nexus::input::Action::ps_button);
    require(shell.control_center_open(), "PS action should open the Control Center overlay");
    shell.handle(nexus::input::Action::move_right);
    shell.handle(nexus::input::Action::accept);
    require(shell.control_center_open(), "selecting a control should not dismiss the overlay");
    require(
        shell.status_message().find("not implemented") != std::string::npos,
        "unsupported system controls should be identified honestly"
    );
    shell.handle(nexus::input::Action::back);
    require(!shell.control_center_open(), "Back should close the overlay");
    require(shell.focused_id() == "app.1", "closing overlay should restore Home focus");
    shell.handle(nexus::input::Action::ps_button);
    shell.handle(nexus::input::Action::options);
    shell.handle(nexus::input::Action::move_right);
    shell.handle(nexus::input::Action::accept);
    require(
        shell.status_message() == "Control position set.",
        "Control Center should support moving and placing a control"
    );
    shell.handle(nexus::input::Action::back);
}

class RecordingCanvas final : public nexus::ui::Canvas {
public:
    void fill_rect(nexus::ui::Rect rect, nexus::ui::Color) override {
        filled_rects.push_back(rect);
    }
    void stroke_rect(nexus::ui::Rect, nexus::ui::Color, int) override {}
    void draw_text(std::string_view text, int, int, int, nexus::ui::Color) override {
        labels.emplace_back(text);
    }
    void draw_line(int, int, int, int, nexus::ui::Color) override {}

    std::vector<std::string> labels;
    std::vector<nexus::ui::Rect> filled_rects;
};

void test_media_section_navigation() {
    nexus::shell::SystemShell shell;
    shell.handle(nexus::input::Action::move_up);
    shell.handle(nexus::input::Action::move_right);
    require(shell.focused_id() == "nav.media", "Home navigation should reach the Media section");
    shell.handle(nexus::input::Action::accept);

    RecordingCanvas canvas;
    shell.render(canvas);
    require(
        std::find(canvas.labels.begin(), canvas.labels.end(), "Media") != canvas.labels.end(),
        "selecting Media should render its own section"
    );
}

void test_game_hub_and_library_navigation() {
    nexus::shell::GameHubScreen direct_game_hub;
    direct_game_hub.mouse_move(540, 250);
    require(
        direct_game_hub.mouse_click(540, 320) &&
            direct_game_hub.status_message().find("No online account") != std::string::npos,
        "Game Hub tabs and contextual actions should respond to mouse input"
    );

    nexus::shell::SystemShell shell;
    shell.handle(nexus::input::Action::move_right);
    shell.handle(nexus::input::Action::accept);
    RecordingCanvas canvas;
    shell.render(canvas);
    require(
        std::find(canvas.labels.begin(), canvas.labels.end(), "GAME HUB  |  LOCAL TEST DATA") !=
            canvas.labels.end(),
        "selecting a local game should open its Game Hub"
    );
    shell.handle(nexus::input::Action::move_right);
    shell.handle(nexus::input::Action::accept);
    require(
        shell.status_message().find("No online account") != std::string::npos,
        "Game Hub service actions should disclose offline limitations"
    );

    nexus::shell::SystemShell library_shell;
    library_shell.handle(nexus::input::Action::move_up);
    library_shell.handle(nexus::input::Action::move_right);
    library_shell.handle(nexus::input::Action::move_right);
    require(library_shell.focused_id() == "nav.library", "Home should navigate to Game Library");
    library_shell.handle(nexus::input::Action::accept);
    require(
        library_shell.focused_id() == "library.app.0",
        "Game Library should focus a local sample entry"
    );
    library_shell.handle(nexus::input::Action::accept);
    canvas.labels.clear();
    library_shell.render(canvas);
    require(
        std::find(canvas.labels.begin(), canvas.labels.end(), "GAME HUB  |  LOCAL TEST DATA") !=
            canvas.labels.end(),
        "opening a library item should show its Game Hub"
    );
}

void test_settings_persistence_and_supported_options() {
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
        ("nexusps5-settings-test-" + std::to_string(unique) + ".ini");
    nexus::core::Config config;
    nexus::shell::SettingsScreen settings(config, path);

    require(settings.handle(nexus::input::Action::accept), "Settings should enter the options pane");
    require(settings.handle(nexus::input::Action::accept), "Fullscreen should be toggleable");
    require(config.fullscreen, "Settings should update the host fullscreen configuration");
    require(settings.take_window_change(), "fullscreen changes should request a host window update");

    require(settings.handle(nexus::input::Action::back), "Settings should return to category navigation");
    for (int index = 0; index < 7; ++index) {
        require(settings.handle(nexus::input::Action::move_down), "Settings categories should be navigable");
    }
    require(settings.handle(nexus::input::Action::accept), "Accessibility options should be focusable");
    require(settings.handle(nexus::input::Action::accept), "High contrast should be toggleable");
    require(config.high_contrast, "Accessibility settings should update high contrast");
    settings.focus_users_and_accounts();
    require(settings.handle(nexus::input::Action::accept), "Local profile name should be editable");
    require(config.user_name == "Player 2", "profile setting should update the local user label");

    nexus::core::Config loaded;
    std::string error;
    require(
        nexus::core::Config::load_from_file(path, loaded, error) &&
            loaded.fullscreen && loaded.high_contrast && loaded.user_name == "Player 2",
        "Settings changes should persist to the configuration file"
    );
    nexus::shell::SettingsScreen mouse_settings(config, path);
    mouse_settings.mouse_move(520, 220);
    require(
        mouse_settings.mouse_click(520, 220) && !config.fullscreen,
        "Settings options should respond to mouse clicks"
    );
    require(
        nexus::core::Config::load_from_file(path, loaded, error) && !loaded.fullscreen,
        "mouse changes should persist to the configuration file"
    );
    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove(path.string() + ".tmp", cleanup_error);
}

void test_profile_settings_update_home_label() {
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
        ("nexusps5-profile-test-" + std::to_string(unique) + ".ini");
    nexus::shell::SystemShell shell({}, path);
    shell.handle(nexus::input::Action::move_up);
    for (int index = 0; index < 4; ++index) {
        shell.handle(nexus::input::Action::move_right);
    }
    require(shell.focused_id() == "nav.profile", "Home should expose a profile access point");
    shell.handle(nexus::input::Action::accept);
    shell.handle(nexus::input::Action::accept);
    shell.handle(nexus::input::Action::back);

    RecordingCanvas canvas;
    shell.render(canvas);
    require(
        std::find(canvas.labels.begin(), canvas.labels.end(), "Player 2") != canvas.labels.end(),
        "updated local profile name should be reflected on Home"
    );
    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove(path.string() + ".tmp", cleanup_error);
}

void test_welcome_hub_widget_management_and_persistence() {
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
        ("nexusps5-welcome-test-" + std::to_string(unique) + ".layout");
    nexus::shell::WelcomeHub hub(path);
    std::string error;
    require(hub.load(error), "a missing Welcome Hub layout should load defaults");
    require(hub.handle(nexus::input::Action::options), "Options should enter widget edit mode");
    require(hub.handle(nexus::input::Action::toggle_widget), "X should toggle selected widget visibility");
    require(hub.handle(nexus::input::Action::cycle_background), "background selection should be editable");
    require(hub.handle(nexus::input::Action::place_widget), "Square should enter widget move mode");
    require(hub.handle(nexus::input::Action::move_right), "widget positions should be movable");
    require(hub.handle(nexus::input::Action::accept), "X should place the moved widget");

    nexus::shell::WelcomeHub restored(path);
    require(restored.load(error), "saved Welcome Hub layout should reload");
    require(restored.background_id() == "nexus-current", "background choice should persist");
    const auto friends = std::find_if(
        restored.widgets().begin(),
        restored.widgets().end(),
        [](const nexus::shell::WelcomeWidget& widget) { return widget.id == "friends"; }
    );
    const auto accessibility = std::find_if(
        restored.widgets().begin(),
        restored.widgets().end(),
        [](const nexus::shell::WelcomeWidget& widget) { return widget.id == "accessibility"; }
    );
    require(friends != restored.widgets().end() && !friends->enabled, "widget visibility should persist");
    require(
        friends != restored.widgets().end() && accessibility != restored.widgets().end() &&
            friends->column == 1 && accessibility->column == 0,
        "widget position changes should persist"
    );

    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove(path.string() + ".tmp", cleanup_error);
}

void test_control_center_animation_and_focus() {
    nexus::ui::ScalarAnimation animation;
    animation.set_target(1.0F, 0.2F);
    animation.update(0.1F);
    require(animation.value() > 0.0F && animation.value() < 1.0F, "scalar animation should interpolate");
    animation.update(0.2F);
    require(animation.value() == 1.0F, "scalar animation should finish at its target");

    nexus::shell::ControlCenterOverlay overlay;
    overlay.open();
    overlay.update(0.09F);
    RecordingCanvas canvas;
    overlay.render(canvas);
    require(
        canvas.filled_rects.size() > 1 && canvas.filled_rects[1].y > 390 &&
            canvas.filled_rects[1].y < 720,
        "Control Center should slide into view"
    );
    overlay.update(0.2F);
    canvas.filled_rects.clear();
    overlay.render(canvas);
    require(canvas.filled_rects[1].y == 390, "Control Center should reach its resting position");
    overlay.close();
    overlay.update(0.09F);
    canvas.filled_rects.clear();
    overlay.render(canvas);
    require(canvas.filled_rects[1].y > 390, "Control Center should animate out when closed");
}

} // namespace

int main() {
    test_focus_navigation();
    test_focus_transition_and_pointer();
    test_shell_navigation_and_control_center();
    test_media_section_navigation();
    test_game_hub_and_library_navigation();
    test_settings_persistence_and_supported_options();
    test_profile_settings_update_home_label();
    test_welcome_hub_widget_management_and_persistence();
    test_control_center_animation_and_focus();
}
