#pragma once

#include "nexus/input/action.hpp"
#include "nexus/ui/canvas.hpp"

#include <string>

namespace nexus::shell {

class GameHubScreen {
public:
    void set_game(std::string title, std::string description, ui::Color artwork);
    [[nodiscard]] bool handle(input::Action action);
    void mouse_move(int x, int y);
    [[nodiscard]] bool mouse_click(int x, int y);
    void render(ui::Canvas& canvas, bool high_contrast = false) const;
    [[nodiscard]] const std::string& status_message() const;

private:
    std::string title_ = "NEXUS SAMPLE 01";
    std::string description_ = "Local interface sample - not an executable";
    ui::Color artwork_{45, 114, 226, 255};
    std::string status_message_;
    int selected_tab_ = 0;
};

} // namespace nexus::shell
