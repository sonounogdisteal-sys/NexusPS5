#include "nexus/input/action.hpp"
#include "nexus/shell/system_shell.hpp"
#include "nexus/ui/canvas.hpp"
#include "nexus/ui/focus_manager.hpp"

#include <algorithm>
#include <cstdlib>
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
}

class RecordingCanvas final : public nexus::ui::Canvas {
public:
    void fill_rect(nexus::ui::Rect, nexus::ui::Color) override {}
    void stroke_rect(nexus::ui::Rect, nexus::ui::Color, int) override {}
    void draw_text(std::string_view text, int, int, int, nexus::ui::Color) override {
        labels.emplace_back(text);
    }
    void draw_line(int, int, int, int, nexus::ui::Color) override {}

    std::vector<std::string> labels;
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

} // namespace

int main() {
    test_focus_navigation();
    test_focus_transition_and_pointer();
    test_shell_navigation_and_control_center();
    test_media_section_navigation();
}
