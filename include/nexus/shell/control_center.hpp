#pragma once

#include "nexus/input/action.hpp"
#include "nexus/ui/component.hpp"

#include <string>

namespace nexus::shell {

class ControlCenterOverlay {
public:
    ControlCenterOverlay();

    void render(ui::Canvas& canvas) const;
    [[nodiscard]] bool handle(input::Action action);
    [[nodiscard]] bool focus_at(int x, int y);
    [[nodiscard]] bool is_open() const;
    void open();
    void close();
    [[nodiscard]] const std::string& status_message() const;

private:
    ui::Scene scene_;
    bool open_ = false;
    std::string status_message_;
};

} // namespace nexus::shell
