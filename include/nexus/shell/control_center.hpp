#pragma once

#include "nexus/input/action.hpp"
#include "nexus/ui/animation.hpp"
#include "nexus/ui/component.hpp"

#include <array>
#include <string>

namespace nexus::shell {

class ControlCenterOverlay {
public:
    ControlCenterOverlay();

    void render(ui::Canvas& canvas, bool high_contrast = false) const;
    void update(float delta_seconds);
    [[nodiscard]] bool handle(input::Action action);
    [[nodiscard]] bool focus_at(int x, int y);
    [[nodiscard]] bool is_open() const;
    void open();
    void close();
    [[nodiscard]] const std::string& status_message() const;

private:
    void rebuild_scene();
    [[nodiscard]] bool move_control(ui::Direction direction);

    ui::Scene scene_;
    ui::ScalarAnimation progress_;
    std::array<std::size_t, 10> control_order_{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    bool open_ = false;
    bool rearranging_ = false;
    bool moving_control_ = false;
    std::string status_message_;
};

} // namespace nexus::shell
