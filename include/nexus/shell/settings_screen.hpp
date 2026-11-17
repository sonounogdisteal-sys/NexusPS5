#pragma once

#include "nexus/core/config.hpp"
#include "nexus/input/action.hpp"
#include "nexus/ui/canvas.hpp"

#include <filesystem>
#include <string>

namespace nexus::shell {

class SettingsScreen {
public:
    SettingsScreen(core::Config& config, std::filesystem::path config_path);

    [[nodiscard]] bool handle(input::Action action);
    void render(ui::Canvas& canvas) const;
    void mouse_move(int x, int y);
    [[nodiscard]] bool mouse_click(int x, int y);
    void focus_users_and_accounts();
    void report_external_error(std::string message);
    [[nodiscard]] bool take_window_change();
    [[nodiscard]] const std::string& status_message() const;

private:
    [[nodiscard]] int item_count() const;
    [[nodiscard]] std::string item_label(int index) const;
    [[nodiscard]] std::string item_value(int index) const;
    [[nodiscard]] std::string item_description(int index) const;
    [[nodiscard]] bool apply_selected_item();
    void set_status(std::string message);

    core::Config& config_;
    std::filesystem::path config_path_;
    std::string status_message_;
    int category_index_ = 0;
    int item_index_ = 0;
    bool options_focused_ = false;
    bool window_change_pending_ = false;
};

} // namespace nexus::shell
