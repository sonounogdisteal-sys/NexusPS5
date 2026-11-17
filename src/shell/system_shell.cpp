#include "nexus/shell/system_shell.hpp"

#include "nexus/ui/theme.hpp"
#include "nexus/ui/icon.hpp"

#include <array>
#include <cmath>
#include <utility>

namespace nexus::shell {
namespace {

using ui::Color;
using ui::Component;
using ui::Rect;

const Color subdued{177, 190, 211, 255};
const Color white{245, 247, 251, 255};
const Color blue{44, 120, 239, 255};

Component text_component(std::string id, std::string label, Rect bounds, int size, Color color) {
    return {
        std::move(id),
        std::move(label),
        bounds,
        {0, 0, 0, 0},
        color,
        size,
        false,
        false,
        false,
    };
}

} // namespace

SystemShell::SystemShell(
    core::Config config,
    std::filesystem::path config_path,
    WelcomeHub welcome_hub
)
    : config_(std::move(config)),
      config_path_(std::move(config_path)),
      settings_screen_(config_, config_path_),
      welcome_hub_(std::move(welcome_hub)),
      background_animation_({26, 78, 151, 255}),
      games_{
          {"Welcome Hub", "Local widget samples - no online services", {35, 101, 209, 255}},
          {"NEXUS SAMPLE 01", "Local interface sample - not an executable", {45, 114, 226, 255}},
          {"NEXUS SAMPLE 02", "Local interface sample - not an executable", {140, 73, 186, 255}},
          {"NEXUS SAMPLE 03", "Local interface sample - not an executable", {25, 143, 132, 255}},
          {"NEXUS SAMPLE 04", "Local interface sample - not an executable", {199, 101, 44, 255}},
          {"NEXUS SAMPLE 05", "Local interface sample - not an executable", {53, 89, 153, 255}},
      },
      media_{
          {"MEDIA SAMPLE 01", "Local media entry - playback unavailable", {154, 80, 146, 255}},
          {"MEDIA SAMPLE 02", "Local media entry - playback unavailable", {40, 134, 159, 255}},
          {"MEDIA SAMPLE 03", "Local media entry - playback unavailable", {174, 106, 52, 255}},
      } {
    welcome_hub_.set_high_contrast(config_.high_contrast);
    rebuild_home_scene();
    rebuild_library_scene();
    set_home_theme();
}

void SystemShell::render(ui::Canvas& canvas) const {
    switch (view_) {
    case View::home:
        render_wallpaper(canvas);
        home_scene_.render(canvas);
        {
            const std::string_view focus = home_scene_.focused_id();
            ui::Icon(ui::IconKind::library).render(
                canvas, {238, 31, 16, 16}, focus == "nav.library" ? white : subdued
            );
            ui::Icon(ui::IconKind::settings).render(
                canvas, {842, 31, 16, 16}, focus == "nav.settings" ? white : subdued
            );
            ui::Icon(ui::IconKind::profile).render(
                canvas, {980, 31, 16, 16}, focus == "nav.profile" ? white : subdued
            );
        }
        if (!showing_media_ && selected_app().title == "Welcome Hub") {
            welcome_hub_.render(canvas);
        } else {
            render_selected_entry(canvas);
        }
        if (status_message_ !=
                "Local shell prototype - guest applications are not executable." &&
            (showing_media_ || selected_app().title != "Welcome Hub")) {
            canvas.draw_text(status_message_, 66, 624, 14, {194, 206, 225, 255});
        }
        canvas.draw_text(
            "Arrows: navigate    Enter: open    P: Control Center    O: widget editor",
            66, 690, 14, {145, 161, 185, 255}
        );
        break;
    case View::game_hub:
        game_hub_.render(canvas, config_.high_contrast);
        break;
    case View::game_library:
        render_game_library(canvas);
        break;
    case View::settings:
        settings_screen_.render(canvas);
        break;
    case View::welcome_hub:
        render_wallpaper(canvas);
        welcome_hub_.render(canvas);
        break;
    }

    control_center_.render(canvas, config_.high_contrast);
}

void SystemShell::update(float delta_seconds) {
    control_center_.update(delta_seconds);
    background_animation_.update(delta_seconds);
}

void SystemShell::handle(input::Action action) {
    using input::Action;
    using ui::Direction;

    if (control_center_.is_open()) {
        if (!control_center_.handle(action) && action == Action::quit) {
            status_message_ = "Quit requested.";
        }
        return;
    }
    if (action == Action::ps_button) {
        control_center_.open();
        return;
    }

    switch (view_) {
    case View::welcome_hub:
        if (welcome_hub_.handle(action)) {
            if (action == Action::cycle_background) {
                background_animation_.set_target(welcome_hub_.background_color(), 0.22F);
            }
            return;
        }
        if (action == Action::back) {
            view_ = View::home;
            set_home_theme();
            return;
        }
        if (action == Action::quit) {
            status_message_ = "Quit requested.";
        }
        return;
    case View::settings:
        if (action == Action::back) {
            view_ = View::home;
            return;
        }
        {
            const std::string previous_user_name = config_.user_name;
            const std::string previous_focus(home_scene_.focused_id());
            if (!settings_screen_.handle(action) && action == Action::quit) {
                status_message_ = "Quit requested.";
            }
            if (config_.user_name != previous_user_name) {
                rebuild_home_scene();
                static_cast<void>(home_scene_.focus(previous_focus));
            }
        }
        welcome_hub_.set_high_contrast(config_.high_contrast);
        return;
    case View::game_library:
        if (action == Action::back) {
            view_ = View::home;
            return;
        }
        if (action == Action::accept) {
            activate_library_component();
            return;
        }
        if (action == Action::move_left) {
            static_cast<void>(library_scene_.move(Direction::left));
        } else if (action == Action::move_right) {
            static_cast<void>(library_scene_.move(Direction::right));
        } else if (action == Action::move_up) {
            static_cast<void>(library_scene_.move(Direction::up));
        } else if (action == Action::move_down) {
            static_cast<void>(library_scene_.move(Direction::down));
        } else if (action == Action::quit) {
            status_message_ = "Quit requested.";
        }
        return;
    case View::game_hub:
        if (action == Action::back) {
            view_ = View::home;
            return;
        }
        if (!game_hub_.handle(action) && action == Action::quit) {
            status_message_ = "Quit requested.";
        }
        return;
    case View::home:
        break;
    }

    switch (action) {
    case Action::move_left:
        move_focus(Direction::left);
        break;
    case Action::move_right:
        move_focus(Direction::right);
        break;
    case Action::move_up:
        move_focus(Direction::up);
        break;
    case Action::move_down:
        move_focus(Direction::down);
        break;
    case Action::accept:
        activate_focused_component();
        break;
    case Action::options:
        if (!showing_media_ && selected_app().title == "Welcome Hub") {
            view_ = View::welcome_hub;
            static_cast<void>(welcome_hub_.handle(Action::options));
        }
        break;
    case Action::back:
        static_cast<void>(home_scene_.focus(showing_media_ ? "nav.media" : "nav.games"));
        break;
    case Action::toggle_widget:
    case Action::place_widget:
    case Action::resize_smaller:
    case Action::resize_larger:
    case Action::cycle_background:
        if (!showing_media_ && selected_app().title == "Welcome Hub") {
            view_ = View::welcome_hub;
            static_cast<void>(welcome_hub_.handle(Action::options));
            static_cast<void>(welcome_hub_.handle(action));
            if (action == Action::cycle_background) {
                background_animation_.set_target(welcome_hub_.background_color(), 0.22F);
            }
        }
        break;
    case Action::ps_button:
    case Action::quit:
        status_message_ = "Quit requested.";
        break;
    }
}

void SystemShell::mouse_move(int x, int y) {
    if (control_center_.is_open()) {
        static_cast<void>(control_center_.focus_at(x, y));
        return;
    }
    switch (view_) {
    case View::game_library:
        static_cast<void>(library_scene_.focus_at(x, y));
        return;
    case View::game_hub:
        game_hub_.mouse_move(x, y);
        return;
    case View::settings:
        settings_screen_.mouse_move(x, y);
        return;
    case View::welcome_hub:
        welcome_hub_.mouse_move(x, y);
        return;
    case View::home:
        if (home_scene_.focus_at(x, y)) {
            const std::string focused(home_scene_.focused_id());
            if (focused.starts_with("app.")) {
                const std::size_t index = static_cast<std::size_t>(std::stoul(focused.substr(4)));
                if (index != selected_app_index_) {
                    select_app(index);
                    rebuild_home_scene();
                    static_cast<void>(home_scene_.focus(focused));
                }
            }
        }
        return;
    }
}

void SystemShell::mouse_click(int x, int y) {
    if (control_center_.is_open()) {
        if (control_center_.focus_at(x, y)) {
            static_cast<void>(control_center_.handle(input::Action::accept));
        }
        return;
    }
    switch (view_) {
    case View::game_library:
        if (library_scene_.focus_at(x, y)) {
            activate_library_component();
        }
        return;
    case View::game_hub:
        static_cast<void>(game_hub_.mouse_click(x, y));
        return;
    case View::settings: {
        const std::string previous_user_name = config_.user_name;
        static_cast<void>(settings_screen_.mouse_click(x, y));
        if (config_.user_name != previous_user_name) {
            const std::string focused(home_scene_.focused_id());
            rebuild_home_scene();
            static_cast<void>(home_scene_.focus(focused));
        }
        welcome_hub_.set_high_contrast(config_.high_contrast);
        return;
    }
    case View::welcome_hub:
        static_cast<void>(welcome_hub_.mouse_click(x, y));
        return;
    case View::home:
        if (home_scene_.focus_at(x, y)) {
            activate_focused_component();
        }
        return;
    }
}

std::optional<SystemShell::WindowSettings> SystemShell::take_window_update() {
    if (!settings_screen_.take_window_change()) {
        return std::nullopt;
    }
    return WindowSettings{config_.render_width, config_.render_height, config_.fullscreen};
}

void SystemShell::report_system_error(std::string message) {
    settings_screen_.report_external_error(message);
    status_message_ = std::move(message);
}

bool SystemShell::control_center_open() const {
    return control_center_.is_open();
}

std::string_view SystemShell::focused_id() const {
    if (control_center_.is_open()) {
        return {};
    }
    if (view_ == View::game_library) {
        return library_scene_.focused_id();
    }
    return view_ == View::home ? home_scene_.focused_id() : std::string_view{};
}

const std::string& SystemShell::status_message() const {
    if (control_center_.is_open()) {
        return control_center_.status_message();
    }
    switch (view_) {
    case View::game_hub:
        return game_hub_.status_message();
    case View::game_library:
        return status_message_;
    case View::settings:
        return settings_screen_.status_message();
    case View::welcome_hub:
        return welcome_hub_.status_message();
    case View::home:
        return status_message_;
    }
    return status_message_;
}

void SystemShell::rebuild_home_scene() {
    std::vector<Component> components;
    components.reserve(16);

    components.push_back({
        "nav.games", "Games", {48, 22, 92, 38}, {0, 0, 0, 0}, showing_media_ ? subdued : white,
        21, true, false, false,
    });
    components.push_back({
        "nav.media", "Media", {146, 22, 92, 38}, {0, 0, 0, 0}, showing_media_ ? white : subdued,
        21, true, false, false,
    });
    components.push_back({
        "nav.library", "Game Library", {258, 22, 150, 38}, {0, 0, 0, 0}, subdued,
        17, true, false, false,
    });
    components.push_back({
        "nav.settings", "Settings", {866, 22, 110, 38}, {0, 0, 0, 0}, subdued,
        17, true, false, false,
    });
    components.push_back({
        "nav.profile", config_.user_name, {1000, 22, 220, 38}, {0, 0, 0, 0}, subdued,
        16, true, false, false,
    });

    const auto& apps = showing_media_ ? media_ : games_;
    int x = 84;
    for (std::size_t index = 0; index < apps.size(); ++index) {
        const bool selected = index == selected_app_index_;
        const int size = selected ? 94 : 62;
        const int y = selected ? 72 : 88;
        const Rect card{x, y, size, size};
        const std::string icon_label = apps[index].title == "Welcome Hub"
            ? "Welcome"
            : (showing_media_ ? "M" : "") +
                apps[index].title.substr(apps[index].title.size() - 2);
        components.push_back({
            "app." + std::to_string(index),
            icon_label,
            card,
            apps[index].color,
            white,
            selected ? 14 : 12,
            true,
            true,
            selected,
        });
        x += size + 8;
    }

    if (showing_media_ || apps[selected_app_index_].title != "Welcome Hub") {
        components.push_back({
            "action.play",
            showing_media_ ? "Open media" : "Game Hub",
            {66, 328, 202, 48},
            blue,
            white,
            19,
            true,
            true,
            false,
        });
    }

    home_scene_.set_components(std::move(components));
    static_cast<void>(home_scene_.focus("app." + std::to_string(selected_app_index_)));
}

void SystemShell::render_wallpaper(ui::Canvas& canvas) const {
    if (config_.high_contrast) {
        canvas.fill_rect({0, 0, 1280, 720}, ui::theme::high_contrast_background);
        for (int wave = 0; wave < 11; ++wave) {
            const double phase = static_cast<double>(wave) * 0.48;
            int previous_x = 0;
            int previous_y = 0;
            for (int x = 0; x <= 1280; x += 32) {
                const int y = 204 + wave * 27 +
                    static_cast<int>(std::sin(static_cast<double>(x) * 0.007 + phase) * 34.0);
                if (x != 0) {
                    canvas.draw_line(previous_x, previous_y, x, y, ui::theme::high_contrast_text);
                }
                previous_x = x;
                previous_y = y;
            }
        }
        return;
    }

    const Color theme = background_animation_.value();
    canvas.fill_rect({0, 0, 1280, 720}, ui::theme::background);
    for (int band = 0; band < 40; ++band) {
        const auto intensity = static_cast<std::uint8_t>(band / 2);
        canvas.fill_rect(
            {480 + band * 20, 0, 20, 720},
            {static_cast<std::uint8_t>(theme.red / 10),
             static_cast<std::uint8_t>(6 + theme.green / 9 + intensity),
             static_cast<std::uint8_t>(16 + theme.blue / 6 + intensity), 255}
        );
    }

    for (int wave = 0; wave < 11; ++wave) {
        const double phase = static_cast<double>(wave) * 0.48;
        int previous_x = 0;
        int previous_y = 0;
        for (int x = 0; x <= 1280; x += 32) {
            const double angle = static_cast<double>(x) * 0.007 + phase;
            const int y = 204 + wave * 27 + static_cast<int>(std::sin(angle) * 34.0);
            if (x != 0) {
                canvas.draw_line(
                    previous_x, previous_y, x, y,
                    {theme.red, theme.green, theme.blue,
                     static_cast<std::uint8_t>(22 + wave * 2)}
                );
            }
            previous_x = x;
            previous_y = y;
        }
    }
}

void SystemShell::render_selected_entry(ui::Canvas& canvas) const {
    const auto& app = selected_app();
    canvas.draw_text(app.title, 66, 194, 31, white);
    canvas.draw_text(app.subtitle, 66, 237, 17, subdued);
    canvas.fill_rect({820, 182, 378, 342}, app.color);
    canvas.fill_rect({820, 182, 378, 3}, {255, 255, 255, 190});
    canvas.fill_rect(
        {858, 220, 302, 266},
        {static_cast<std::uint8_t>(app.color.red / 2),
         static_cast<std::uint8_t>(app.color.green / 2),
         static_cast<std::uint8_t>(app.color.blue / 2), 255}
    );
    canvas.draw_text("NEXUS", 950, 313, 34, white);
    canvas.draw_text("ORIGINAL LOCAL ART", 906, 358, 15, subdued);
    canvas.draw_text(
        showing_media_
            ? "Media playback is unavailable in this shell prototype."
            : "Guest execution is unavailable; no CPU, kernel, or loader is implemented.",
        66, 397, 15, subdued
    );
}

void SystemShell::activate_focused_component() {
    const std::string focused(home_scene_.focused_id());
    if (focused == "nav.games" && showing_media_) {
        showing_media_ = false;
        selected_app_index_ = 0;
        set_home_theme();
        rebuild_home_scene();
        return;
    }
    if (focused == "nav.media" && !showing_media_) {
        showing_media_ = true;
        selected_app_index_ = 0;
        set_home_theme();
        rebuild_home_scene();
        return;
    }
    if (focused == "nav.settings") {
        view_ = View::settings;
        return;
    }
    if (focused == "nav.library") {
        view_ = View::game_library;
        static_cast<void>(library_scene_.focus("library.app.0"));
        return;
    }
    if (focused == "nav.profile") {
        view_ = View::settings;
        settings_screen_.focus_users_and_accounts();
        return;
    }

    if (focused.starts_with("app.")) {
        const std::size_t index = static_cast<std::size_t>(std::stoul(focused.substr(4)));
        const std::size_t count = showing_media_ ? media_.size() : games_.size();
        if (index < count) {
            select_app(index);
            if (!showing_media_ && games_[index].title == "Welcome Hub") {
                view_ = View::welcome_hub;
                status_message_ = "Welcome Hub preview uses local, non-interactive sample widgets.";
            } else {
                const auto& app = selected_app();
                game_hub_.set_game(app.title, app.subtitle, app.color);
                view_ = View::game_hub;
            }
            rebuild_home_scene();
        }
        return;
    }
    if (focused == "action.play") {
        if (showing_media_) {
            status_message_ = "Media playback is unavailable in this shell prototype.";
        } else {
            const auto& app = selected_app();
            game_hub_.set_game(app.title, app.subtitle, app.color);
            view_ = View::game_hub;
        }
    }
}

void SystemShell::activate_library_component() {
    const std::string focused(library_scene_.focused_id());
    if (!focused.starts_with("library.app.")) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(std::stoul(focused.substr(12)));
    if (index >= games_.size() - 1) {
        return;
    }
    const auto& app = games_[index + 1];
    game_hub_.set_game(app.title, app.subtitle, app.color);
    view_ = View::game_hub;
}

void SystemShell::render_game_library(ui::Canvas& canvas) const {
    const Color background = config_.high_contrast
        ? ui::theme::high_contrast_background
        : ui::theme::background;
    const Color foreground = config_.high_contrast
        ? ui::theme::high_contrast_text
        : ui::theme::text;
    const Color subdued_color = config_.high_contrast
        ? ui::theme::high_contrast_text
        : ui::theme::subdued_text;
    canvas.fill_rect({0, 0, 1280, 720}, background);
    canvas.draw_text("Game Library", 50, 42, 30, foreground);
    canvas.draw_text("LOCAL TEST ENTRIES  |  NO EXECUTABLES INSTALLED", 50, 91, 13, subdued_color);
    library_scene_.render(canvas);
    canvas.draw_text(
        "Arrows: navigate    Enter: open Game Hub    Esc: return to Home",
        52, 674, 14, subdued_color
    );
}

void SystemShell::rebuild_library_scene() {
    std::vector<Component> components;
    components.reserve(games_.size() - 1);
    for (std::size_t index = 1; index < games_.size(); ++index) {
        const std::size_t cell = index - 1;
        const int x = 56 + static_cast<int>(cell % 3) * 396;
        const int y = 154 + static_cast<int>(cell / 3) * 214;
        components.push_back({
            "library.app." + std::to_string(cell),
            games_[index].title,
            {x, y, 350, 166},
            games_[index].color,
            white,
            19,
            true,
            true,
            false,
        });
        components.push_back(text_component(
            "library.detail." + std::to_string(cell),
            games_[index].subtitle,
            {x + 18, y + 56, 320, 80},
            13,
            subdued
        ));
    }
    library_scene_.set_components(std::move(components));
    static_cast<void>(library_scene_.focus("library.app.0"));
}

void SystemShell::move_focus(ui::Direction direction) {
    const std::string previous_focus(home_scene_.focused_id());
    if (!home_scene_.move(direction)) {
        return;
    }
    const std::string current_focus(home_scene_.focused_id());
    if (current_focus.starts_with("app.")) {
        const std::size_t index = static_cast<std::size_t>(std::stoul(current_focus.substr(4)));
        if (index != selected_app_index_) {
            select_app(index);
            rebuild_home_scene();
            static_cast<void>(home_scene_.focus(current_focus));
        }
    } else if (previous_focus.starts_with("app.") && current_focus == "nav.media") {
        showing_media_ = true;
        selected_app_index_ = 0;
        set_home_theme();
        rebuild_home_scene();
        static_cast<void>(home_scene_.focus("nav.media"));
    } else if (previous_focus.starts_with("app.") && current_focus == "nav.games") {
        showing_media_ = false;
        selected_app_index_ = 0;
        set_home_theme();
        rebuild_home_scene();
        static_cast<void>(home_scene_.focus("nav.games"));
    }
}

void SystemShell::select_app(std::size_t index) {
    selected_app_index_ = index;
    set_home_theme();
}

void SystemShell::set_home_theme() {
    const Color target = !showing_media_ && selected_app().title == "Welcome Hub"
        ? welcome_hub_.background_color()
        : selected_app().color;
    background_animation_.set_target(target, 0.22F);
}

const SystemShell::SampleApp& SystemShell::selected_app() const {
    const auto& apps = showing_media_ ? media_ : games_;
    return apps.at(selected_app_index_);
}

} // namespace nexus::shell
