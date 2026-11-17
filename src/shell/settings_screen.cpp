#include "nexus/shell/settings_screen.hpp"

#include "nexus/ui/theme.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace nexus::shell {
namespace {

constexpr std::array<std::string_view, 9> categories{
    "System",
    "Users and Accounts",
    "Storage",
    "Sound",
    "Screen and Video",
    "Accessories",
    "Network",
    "Accessibility",
    "Saved Data and Game/App Settings",
};

} // namespace

SettingsScreen::SettingsScreen(core::Config& config, std::filesystem::path config_path)
    : config_(config), config_path_(std::move(config_path)) {}

bool SettingsScreen::handle(input::Action action) {
    using input::Action;
    switch (action) {
    case Action::move_up:
        if (options_focused_) {
            if (item_index_ > 0) {
                --item_index_;
            }
        } else if (category_index_ > 0) {
            --category_index_;
            item_index_ = 0;
        }
        return true;
    case Action::move_down:
        if (options_focused_) {
            if (item_index_ + 1 < item_count()) {
                ++item_index_;
            }
        } else if (category_index_ + 1 < static_cast<int>(categories.size())) {
            ++category_index_;
            item_index_ = 0;
        }
        return true;
    case Action::move_left:
        if (options_focused_) {
            options_focused_ = false;
        }
        return true;
    case Action::move_right:
        if (!options_focused_) {
            options_focused_ = true;
            item_index_ = 0;
        } else {
            return apply_selected_item();
        }
        return true;
    case Action::accept:
        if (!options_focused_) {
            options_focused_ = true;
            item_index_ = 0;
            return true;
        }
        return apply_selected_item();
    case Action::back:
        options_focused_ = false;
        set_status("Settings preview.");
        return true;
    case Action::ps_button:
    case Action::options:
    case Action::place_widget:
    case Action::toggle_widget:
    case Action::resize_smaller:
    case Action::resize_larger:
    case Action::cycle_background:
    case Action::quit:
        return false;
    }
    return false;
}

void SettingsScreen::render(ui::Canvas& canvas) const {
    const bool high_contrast = config_.high_contrast;
    const ui::Color background = high_contrast
        ? ui::theme::high_contrast_background
        : ui::theme::background;
    const ui::Color foreground = high_contrast
        ? ui::theme::high_contrast_text
        : ui::theme::text;
    const ui::Color subdued = high_contrast
        ? ui::theme::high_contrast_text
        : ui::theme::subdued_text;
    canvas.fill_rect({0, 0, 1280, 720}, background);
    canvas.draw_text("Settings", 48, 34, 30, foreground);
    canvas.draw_text("LOCAL CONFIGURATION  |  UNSUPPORTED SERVICES ARE LABELED", 48, 83, 13, subdued);

    for (std::size_t index = 0; index < categories.size(); ++index) {
        const int y = 132 + static_cast<int>(index) * 51;
        const bool selected = static_cast<int>(index) == category_index_;
        if (selected) {
            canvas.fill_rect(
                {38, y - 5, 394, 42},
                options_focused_ ? ui::Color{22, 32, 48, 255} : ui::theme::accent
            );
        }
        canvas.draw_text(
            categories[index], 56, y + 3, 15,
            selected || high_contrast ? foreground : subdued
        );
    }

    canvas.fill_rect({462, 124, 770, 476}, high_contrast ? ui::Color{0, 0, 0, 255} : ui::theme::panel);
    canvas.draw_text(categories[static_cast<std::size_t>(category_index_)], 496, 151, 22, foreground);
    const int count = item_count();
    for (int index = 0; index < count; ++index) {
        const int y = 212 + index * 95;
        const bool selected = options_focused_ && index == item_index_;
        if (selected) {
            canvas.fill_rect({488, y - 8, 714, 77}, high_contrast
                ? ui::Color{38, 38, 38, 255}
                : ui::Color{30, 43, 64, 255});
            canvas.stroke_rect({488, y - 8, 714, 77}, ui::theme::focus, 2);
        }
        canvas.draw_text(item_label(index), 510, y, 17, foreground);
        canvas.draw_text(item_value(index), 510, y + 29, 14, subdued);
        canvas.draw_text(item_description(index), 510, y + 49, 12, subdued);
    }
    canvas.draw_text(
        status_message_.empty()
            ? "Arrows: navigate    Enter: select    Esc: return"
            : status_message_,
        48, 654, 14, foreground
    );
}

void SettingsScreen::mouse_move(int x, int y) {
    if (x >= 38 && x < 432 && y >= 127 && y < 585) {
        category_index_ = std::min((y - 127) / 51, static_cast<int>(categories.size()) - 1);
        item_index_ = 0;
        options_focused_ = false;
        return;
    }
    if (x >= 488 && x < 1202 && y >= 204) {
        const int index = (y - 204) / 95;
        if (index < item_count() && y < 281 + index * 95) {
            item_index_ = index;
            options_focused_ = true;
        }
    }
}

bool SettingsScreen::mouse_click(int x, int y) {
    if (x >= 38 && x < 432 && y >= 127 && y < 585) {
        mouse_move(x, y);
        return true;
    }
    if (x >= 488 && x < 1202 && y >= 204) {
        const int index = (y - 204) / 95;
        if (index < item_count() && y < 281 + index * 95) {
            mouse_move(x, y);
            return apply_selected_item();
        }
    }
    return false;
}

void SettingsScreen::focus_users_and_accounts() {
    category_index_ = 1;
    item_index_ = 0;
    options_focused_ = true;
}

void SettingsScreen::report_external_error(std::string message) {
    set_status(std::move(message));
}

bool SettingsScreen::take_window_change() {
    return std::exchange(window_change_pending_, false);
}

const std::string& SettingsScreen::status_message() const {
    return status_message_;
}

int SettingsScreen::item_count() const {
    switch (category_index_) {
    case 0: return 1;
    case 1: return 1;
    case 7: return 2;
    case 4: return 1;
    default: return 1;
    }
}

std::string SettingsScreen::item_label(int index) const {
    switch (category_index_) {
    case 0:
        return index == 0 ? "Fullscreen" : "";
    case 1:
        return index == 0 ? "Local profile name" : "";
    case 4:
        return index == 0 ? "Render size" : "";
    case 7:
        return index == 0 ? "High contrast" : "Motion reduction";
    case 2:
        return "Console storage";
    case 3:
        return "Sound output";
    case 5:
        return "Controller settings";
    case 6:
        return "Network connection";
    case 8:
        return "Saved data and game presets";
    default:
        return "";
    }
}

std::string SettingsScreen::item_value(int index) const {
    switch (category_index_) {
    case 0:
        return config_.fullscreen ? "On" : "Off";
    case 1:
        return config_.user_name;
    case 4:
        return std::to_string(config_.render_width) + " x " + std::to_string(config_.render_height);
    case 7:
        return index == 0
            ? (config_.high_contrast ? "On" : "Off")
            : "Unsupported - reduce-motion behavior is not implemented";
    default:
        return "Unsupported - no console service is implemented";
    }
}

std::string SettingsScreen::item_description(int index) const {
    switch (category_index_) {
    case 0:
        return "Toggles fullscreen for the NEXUS host window.";
    case 1:
        return "A local NEXUS profile label; no PlayStation Network account.";
    case 4:
        return "Changes the host window size; logical UI coordinates remain 1280 x 720.";
    case 7:
        return index == 0
            ? "Use a high-contrast palette in NEXUS settings."
            : "This system accessibility service is not implemented.";
    default:
        return "This setting is unsupported by the independent shell.";
    }
}

bool SettingsScreen::apply_selected_item() {
    if ((category_index_ == 7 && item_index_ == 1) ||
        (category_index_ != 0 && category_index_ != 1 &&
         category_index_ != 4 && category_index_ != 7)) {
        set_status("Unsupported setting: no matching host or guest service is implemented.");
        return true;
    }
    if (category_index_ == 7 && item_index_ != 0) {
        set_status("Motion reduction is unsupported.");
        return true;
    }

    core::Config updated = config_;
    if (category_index_ == 0) {
        updated.fullscreen = !updated.fullscreen;
        window_change_pending_ = true;
    } else if (category_index_ == 1) {
        updated.user_name = updated.user_name == "Player" ? "Player 2" : "Player";
    } else if (category_index_ == 4) {
        const bool use_hd = updated.render_width == 1280;
        updated.render_width = use_hd ? 1920 : 1280;
        updated.render_height = use_hd ? 1080 : 720;
        window_change_pending_ = true;
    } else if (category_index_ == 7) {
        updated.high_contrast = !updated.high_contrast;
    }

    std::string error;
    if (!updated.save_to_file(config_path_, error)) {
        window_change_pending_ = false;
        set_status("Setting not saved: " + error);
        return false;
    }
    config_ = std::move(updated);
    set_status(item_label(item_index_) + " updated.");
    return true;
}

void SettingsScreen::set_status(std::string message) {
    status_message_ = std::move(message);
}

} // namespace nexus::shell
