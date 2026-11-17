#pragma once

#include "nexus/input/action.hpp"
#include "nexus/ui/component.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace nexus::shell {

struct WelcomeWidget {
    std::string id;
    std::string title;
    std::string detail;
    int column = 0;
    int row = 0;
    bool enabled = true;
    bool large = false;
};

class WelcomeHub {
public:
    explicit WelcomeHub(std::filesystem::path layout_path = {});

    [[nodiscard]] bool load(std::string& error);
    [[nodiscard]] bool handle(input::Action action);
    void mouse_move(int x, int y);
    [[nodiscard]] bool mouse_click(int x, int y);
    void render(ui::Canvas& canvas) const;
    void set_high_contrast(bool enabled);

    [[nodiscard]] bool editing() const;
    [[nodiscard]] const std::string& status_message() const;
    [[nodiscard]] const std::vector<WelcomeWidget>& widgets() const;
    [[nodiscard]] std::string_view background_id() const;
    [[nodiscard]] ui::Color background_color() const;

private:
    void rebuild_scene();
    void set_status(std::string message);
    [[nodiscard]] std::size_t selected_widget_index() const;
    [[nodiscard]] bool save_layout();
    [[nodiscard]] bool move_selected_widget(ui::Direction direction);
    [[nodiscard]] bool move_widget_or_focus(ui::Direction direction);

    std::filesystem::path layout_path_;
    std::vector<WelcomeWidget> widgets_;
    std::vector<std::string> backgrounds_;
    std::size_t background_index_ = 0;
    std::string selected_widget_id_ = "friends";
    std::string status_message_;
    ui::Scene scene_;
    bool editing_ = false;
    bool moving_widget_ = false;
    bool high_contrast_ = false;
};

} // namespace nexus::shell
