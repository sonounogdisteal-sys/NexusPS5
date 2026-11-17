#include "nexus/shell/control_center.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace nexus::shell {
namespace {

using ui::Color;
using ui::Component;
using ui::Rect;

std::vector<Component> control_components(const std::array<std::size_t, 10>& control_order) {
    constexpr std::array<std::string_view, 10> controls{
        "Home", "Switcher", "Notifications", "Game Base", "Music",
        "Sound", "Mic", "Accessories", "Profile", "Power",
    };
    std::vector<Component> components;
    components.reserve(controls.size());
    constexpr int first_x = 34;
    constexpr int width = 116;
    constexpr int gap = 8;
    for (std::size_t slot = 0; slot < control_order.size(); ++slot) {
        const std::size_t index = control_order[slot];
        const int x = first_x + static_cast<int>(slot) * (width + gap);
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
    rebuild_scene();
}

void ControlCenterOverlay::render(ui::Canvas& canvas, bool high_contrast) const {
    const float progress = progress_.value();
    if (progress <= 0.001F) {
        return;
    }

    const int vertical_offset = static_cast<int>(std::lround((1.0F - progress) * 330.0F));
    const auto backdrop_alpha = static_cast<std::uint8_t>(176.0F * progress);
    canvas.fill_rect({0, 0, 1280, 720}, {0, 4, 14, backdrop_alpha});
    canvas.fill_rect(
        {0, 390 + vertical_offset, 1280, 330 - vertical_offset},
        high_contrast
            ? Color{0, 0, 0, static_cast<std::uint8_t>(255.0F * progress)}
            : Color{13, 23, 40, static_cast<std::uint8_t>(245.0F * progress)}
    );
    const Color foreground = high_contrast ? Color{255, 255, 255, 255} : Color{190, 207, 231, 255};
    canvas.draw_text("CONTROL CENTER", 48, 414 + vertical_offset, 18, foreground);

    constexpr std::array<std::string_view, 4> cards{
        "ACTIVITY  |  Local demo data",
        "TROPHIES  |  No trophies earned",
        "VOICE CHAT  |  Not connected",
        "MUSIC  |  No playback service",
    };
    const Color card_color = high_contrast
        ? Color{0, 0, 0, 255}
        : Color{29, 47, 73, 255};
    for (std::size_t index = 0; index < cards.size(); ++index) {
        const int x = 48 + static_cast<int>(index) * 302;
        canvas.fill_rect(
            {x, 445 + vertical_offset, 280, 98},
            {card_color.red, card_color.green, card_color.blue,
             static_cast<std::uint8_t>(255.0F * progress)}
        );
        canvas.draw_text(
            cards[index], x + 14, 478 + vertical_offset, 16,
            high_contrast ? Color{255, 255, 255, 255} : Color{225, 233, 245, 255}
        );
    }

    scene_.render(canvas, vertical_offset);
    canvas.draw_text(
        status_message_.empty()
            ? "D-pad: navigate    Confirm: select    PS / Esc: close"
            : status_message_,
        48,
        std::min(665 + vertical_offset, 700),
        14,
        high_contrast ? Color{255, 255, 255, 255} : Color{180, 193, 214, 255}
    );
}

void ControlCenterOverlay::update(float delta_seconds) {
    progress_.update(delta_seconds);
}

bool ControlCenterOverlay::handle(input::Action action) {
    using input::Action;
    using ui::Direction;

    switch (action) {
    case Action::move_left:
        if (moving_control_) {
            return move_control(Direction::left);
        }
        return scene_.move(Direction::left);
    case Action::move_right:
        if (moving_control_) {
            return move_control(Direction::right);
        }
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
        if (moving_control_) {
            moving_control_ = false;
            status_message_ = "Control position set.";
            return true;
        }
        const std::string_view focused = scene_.focused_id();
        if (focused == "control.0") {
            close();
            return true;
        }
        status_message_ =
            "Local prototype only - system service for this control is not implemented.";
        return true;
    }
    case Action::options:
        rearranging_ = !rearranging_;
        moving_control_ = rearranging_;
        status_message_ = rearranging_
            ? "Rearrange controls: arrows move an icon; X places it. Hidden controls are unavailable."
            : "Control order set.";
        return true;
    case Action::place_widget:
        if (rearranging_) {
            moving_control_ = true;
            status_message_ = "Move mode: use arrows, then press X to place.";
            return true;
        }
        return false;
    case Action::quit:
        return false;
    case Action::toggle_widget:
    case Action::resize_smaller:
    case Action::resize_larger:
    case Action::cycle_background:
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
    progress_.set_target(1.0F, 0.18F);
    status_message_.clear();
    static_cast<void>(scene_.focus("control.0"));
}

void ControlCenterOverlay::close() {
    open_ = false;
    rearranging_ = false;
    moving_control_ = false;
    progress_.set_target(0.0F, 0.18F);
}

const std::string& ControlCenterOverlay::status_message() const {
    return status_message_;
}

void ControlCenterOverlay::rebuild_scene() {
    scene_.set_components(control_components(control_order_));
}

bool ControlCenterOverlay::move_control(ui::Direction direction) {
    if (direction != ui::Direction::left && direction != ui::Direction::right) {
        return true;
    }
    const std::string focused(scene_.focused_id());
    if (!focused.starts_with("control.")) {
        return false;
    }
    const std::size_t control_index = static_cast<std::size_t>(std::stoul(focused.substr(8)));
    const auto position = std::find(control_order_.begin(), control_order_.end(), control_index);
    if (position == control_order_.end()) {
        return false;
    }
    const auto offset = static_cast<std::ptrdiff_t>(position - control_order_.begin());
    const auto adjacent = offset + (direction == ui::Direction::right ? 1 : -1);
    if (adjacent < 0 || adjacent >= static_cast<std::ptrdiff_t>(control_order_.size())) {
        return false;
    }
    std::swap(control_order_[static_cast<std::size_t>(offset)],
              control_order_[static_cast<std::size_t>(adjacent)]);
    rebuild_scene();
    static_cast<void>(scene_.focus(focused));
    status_message_ = "Moved control icon.";
    return true;
}

} // namespace nexus::shell
