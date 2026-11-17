#pragma once

namespace nexus::input {

enum class Action {
    move_left,
    move_right,
    move_up,
    move_down,
    accept,
    back,
    ps_button,
    options,
    place_widget,
    toggle_widget,
    resize_smaller,
    resize_larger,
    cycle_background,
    quit,
};

} // namespace nexus::input
