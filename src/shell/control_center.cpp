#include "nexus/shell/control_center.hpp"

#include <array>

namespace nexus::shell {
namespace {

using ui::Color;
using ui::Component;
using ui::Rect;

std::vector<Component> control_components() {
    constexpr std::array<std::string_view, 10> controls{
        "Home", "Switcher", "Notifications", "Game Base", "Music",
        "Sound", "Mic", "Accessories", "Profile", "Power",
    };
    std::vector<Component> components;
    components.reserve(controls.size());
    constexpr int first_x = 34;
    constexpr int width = 116;
    constexpr int gap = 8;
    for (std::size_t index = 0; index < controls.size(); ++index) {
        const int x = first_x + static_cast<int>(index) * (width + gap);
        components.push_back({
            "control." + std::to_string(index),
            std::string(controls[index]),
            Rect{x, 570, width, 70},
            Color{30, 43, 64, 255},
            Color{238, 241, 247, 255},
            16,
            true,
            true,
            false,
        });
    }
    return components;
}

} // namespace

ControlCenterOverlay::ControlCenterOverlay() {
    scene_.set_components(control_components());
}

void ControlCenterOverlay::render(ui::Canvas& canvas) const {
    if (!open_) {
        return;
    }

    canvas.fill_rect({0, 0, 1280, 720}, {0, 4, 14, 176});
    canvas.fill_rect({0, 390, 1280, 330}, {13, 23, 40, 245});
    canvas.draw_text("CONTROL CENTER", 48, 414, 18, {190, 207, 231, 255});

    constexpr std::array<std::string_view, 4> cards{
        "ACTIVITY  |  Local demo data",
        "TROPHIES  |  No trophies earned",
        "VOICE CHAT  |  Not connected",
        "MUSIC  |  No playback service",
    };
    constexpr std::array<Color, 4> card_colors{
        Color{29, 47, 73, 255},
        Color{29, 47, 73, 255},
        Color{29, 47, 73, 255},
        Color{29, 47, 73, 255},
    };
    for (std::size_t index = 0; index < cards.size(); ++index) {
        const int x = 48 + static_cast<int>(index) * 302;
        canvas.fill_rect({x, 445, 280, 98}, card_colors[index]);
        canvas.draw_text(cards[index], x + 14, 478, 16, {225, 233, 245, 255});
    }

    scene_.render(canvas);
    canvas.draw_text(
        status_message_.empty()
            ? "Directional buttons: navigate    X: select    PS / Esc: close"
            : status_message_,
        48,
        665,
        14,
        {180, 193, 214, 255}
    );
}

bool ControlCenterOverlay::handle(input::Action action) {
    using input::Action;
    using ui::Direction;

    switch (action) {
    case Action::move_left:
        return scene_.move(Direction::left);
    case Action::move_right:
        return scene_.move(Direction::right);
    case Action::move_up:
        return true;
    case Action::move_down:
        return true;
    case Action::back:
    case Action::ps_button:
        close();
        return true;
    case Action::accept: {
        const std::string_view focused = scene_.focused_id();
        if (focused == "control.0") {
            close();
            return true;
        }
        status_message_ =
            "Local prototype only - system service for this control is not implemented.";
        return true;
    }
    case Action::quit:
        return false;
    }
    return false;
}

bool ControlCenterOverlay::focus_at(int x, int y) {
    return open_ && scene_.focus_at(x, y);
}

bool ControlCenterOverlay::is_open() const {
    return open_;
}

void ControlCenterOverlay::open() {
    open_ = true;
    status_message_.clear();
    static_cast<void>(scene_.focus("control.0"));
}

void ControlCenterOverlay::close() {
    open_ = false;
}

const std::string& ControlCenterOverlay::status_message() const {
    return status_message_;
}

} // namespace nexus::shell
