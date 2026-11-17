#include "nexus/shell/system_shell.hpp"

#include <array>
#include <cmath>
#include <utility>

namespace nexus::shell {
namespace {

using ui::Color;
using ui::Component;
using ui::Rect;

const Color background{5, 10, 23, 255};
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

SystemShell::SystemShell()
    : games_{
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
    rebuild_home_scene();
}

void SystemShell::render(ui::Canvas& canvas) const {
    render_wallpaper(canvas);
    home_scene_.render(canvas);
    if (!showing_media_ && selected_app().title == "Welcome Hub") {
        render_welcome_hub(canvas);
    } else {
        render_selected_entry(canvas);
    }

    if (status_message_ !=
        "Local shell prototype - guest applications are not executable.") {
        canvas.draw_text(status_message_, 66, 624, 14, {194, 206, 225, 255});
    }
    canvas.draw_text(
        "D-pad / arrows: navigate     X / Enter: select     PS / P: Control Center",
        66, 690,
        14,
        {145, 161, 185, 255}
    );

    control_center_.render(canvas);
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
    case Action::ps_button:
        control_center_.open();
        break;
    case Action::back:
        static_cast<void>(home_scene_.focus(showing_media_ ? "nav.media" : "nav.games"));
        break;
    case Action::quit:
        status_message_ = "Quit requested.";
        break;
    }
}

void SystemShell::mouse_move(int x, int y) {
    if (control_center_.is_open()) {
        static_cast<void>(control_center_.focus_at(x, y));
    } else {
        if (home_scene_.focus_at(x, y)) {
            const std::string focused(home_scene_.focused_id());
            if (focused.starts_with("app.")) {
                const std::size_t index = static_cast<std::size_t>(std::stoul(focused.substr(4)));
                if (index != selected_app_index_) {
                    selected_app_index_ = index;
                    rebuild_home_scene();
                    static_cast<void>(home_scene_.focus(focused));
                }
            }
        }
    }
}

void SystemShell::mouse_click(int x, int y) {
    if (control_center_.is_open()) {
        if (control_center_.focus_at(x, y)) {
            static_cast<void>(control_center_.handle(input::Action::accept));
        }
        return;
    }
    if (home_scene_.focus_at(x, y)) {
        activate_focused_component();
    }
}

bool SystemShell::control_center_open() const {
    return control_center_.is_open();
}

std::string_view SystemShell::focused_id() const {
    return control_center_.is_open() ? std::string_view{} : home_scene_.focused_id();
}

const std::string& SystemShell::status_message() const {
    return control_center_.is_open() ? control_center_.status_message() : status_message_;
}

void SystemShell::rebuild_home_scene() {
    std::vector<Component> components;
    components.reserve(14);

    components.push_back({
        "nav.games", "Games", {48, 22, 92, 38}, {0, 0, 0, 0}, showing_media_ ? subdued : white,
        21, true, false, false,
    });
    components.push_back({
        "nav.media", "Media", {146, 22, 92, 38}, {0, 0, 0, 0}, showing_media_ ? white : subdued,
        21, true, false, false,
    });
    components.push_back(text_component(
        "system.status", "LOCAL USER  |  OFFLINE", {970, 22, 260, 38}, 15, subdued
    ));

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
            showing_media_ ? "Open media" : "Play",
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
    canvas.fill_rect({0, 0, 1280, 720}, background);
    const Color theme = selected_app().title == "Welcome Hub"
        ? Color{26, 78, 151, 255}
        : selected_app().color;
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
                    previous_x,
                    previous_y,
                    x,
                    y,
                    {theme.red, theme.green, theme.blue,
                     static_cast<std::uint8_t>(22 + wave * 2)}
                );
            }
            previous_x = x;
            previous_y = y;
        }
    }
}

void SystemShell::render_welcome_hub(ui::Canvas& canvas) const {
    canvas.draw_text("Welcome", 42, 170, 23, white);

    constexpr Color card{12, 19, 32, 235};
    constexpr Color card_heading{230, 235, 244, 255};
    constexpr Color card_detail{177, 190, 211, 255};

    canvas.fill_rect({54, 314, 336, 205}, card);
    canvas.draw_text("Friends Online", 76, 336, 17, card_heading);
    canvas.draw_text("Local sample: 3 profiles offline", 76, 384, 15, card_detail);
    canvas.draw_text("No account or network connection", 76, 411, 14, card_detail);

    canvas.fill_rect({54, 532, 336, 116}, card);
    canvas.draw_text("PlayStation Store", 76, 550, 17, card_heading);
    canvas.draw_text("Unavailable - no online services", 76, 594, 14, card_detail);

    canvas.fill_rect({408, 314, 328, 102}, card);
    canvas.draw_text("Accessibility", 430, 334, 17, card_heading);
    canvas.draw_text("Settings are not implemented", 430, 372, 14, card_detail);

    canvas.fill_rect({408, 429, 328, 219}, card);
    canvas.draw_text("Trophies", 430, 449, 17, card_heading);
    canvas.draw_text("Local test profile", 430, 491, 14, card_detail);
    canvas.draw_text("No trophies earned", 430, 518, 14, card_detail);

    canvas.fill_rect({754, 314, 470, 102}, card);
    canvas.draw_text("Battery", 776, 334, 17, card_heading);
    canvas.draw_text("No controller battery data available", 776, 372, 14, card_detail);

    canvas.fill_rect({754, 429, 470, 219}, card);
    canvas.draw_text("Wishlist", 776, 449, 17, card_heading);
    canvas.draw_text("Local sample only - not connected to PlayStation Store", 776, 491, 14, card_detail);
    canvas.draw_text("Widget services are not implemented", 776, 518, 14, card_detail);
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
        66,
        397,
        15,
        subdued
    );
}

void SystemShell::activate_focused_component() {
    const std::string focused(home_scene_.focused_id());
    if (focused == "nav.games" && showing_media_) {
        showing_media_ = false;
        selected_app_index_ = 0;
        rebuild_home_scene();
        return;
    }
    if (focused == "nav.media" && !showing_media_) {
        showing_media_ = true;
        selected_app_index_ = 0;
        rebuild_home_scene();
        return;
    }

    if (focused.starts_with("app.")) {
        const std::size_t index = static_cast<std::size_t>(std::stoul(focused.substr(4)));
        const std::size_t count = showing_media_ ? media_.size() : games_.size();
        if (index < count) {
            selected_app_index_ = index;
            status_message_ = !showing_media_ && games_[index].title == "Welcome Hub"
                ? "Welcome Hub preview uses local, non-interactive sample widgets."
                : "Selected local sample only; no application is installed or launched.";
            rebuild_home_scene();
        }
        return;
    }

    if (focused == "action.play") {
        status_message_ = showing_media_
            ? "Media playback is unavailable in this shell prototype."
            : "Guest execution is unavailable; no CPU, kernel, or loader is implemented.";
    }
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
            selected_app_index_ = index;
            rebuild_home_scene();
            static_cast<void>(home_scene_.focus(current_focus));
        }
    } else if (previous_focus.starts_with("app.") && current_focus == "nav.media") {
        showing_media_ = true;
        selected_app_index_ = 0;
        rebuild_home_scene();
        static_cast<void>(home_scene_.focus("nav.media"));
    } else if (previous_focus.starts_with("app.") && current_focus == "nav.games") {
        showing_media_ = false;
        selected_app_index_ = 0;
        rebuild_home_scene();
        static_cast<void>(home_scene_.focus("nav.games"));
    }
}

const SystemShell::SampleApp& SystemShell::selected_app() const {
    const auto& apps = showing_media_ ? media_ : games_;
    return apps.at(selected_app_index_);
}

} // namespace nexus::shell
