#include "nexus/platform/sdl_input.hpp"

#include <SDL.h>

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void test_keyboard_mapping() {
    nexus::input::Action action{};
    require(
        nexus::platform::SdlInput::keyboard_action(SDLK_RIGHT, action) &&
            action == nexus::input::Action::move_right,
        "right arrow should map to right navigation"
    );
    require(
        nexus::platform::SdlInput::keyboard_action(SDLK_RETURN, action) &&
            action == nexus::input::Action::accept,
        "Enter should map to accept"
    );
    require(
        nexus::platform::SdlInput::keyboard_action(SDLK_p, action) &&
            action == nexus::input::Action::ps_button,
        "P should map to the PS button action"
    );
    require(
        nexus::platform::SdlInput::keyboard_action(SDLK_o, action) &&
            action == nexus::input::Action::options,
        "O should map to widget edit options"
    );
    require(
        nexus::platform::SdlInput::keyboard_action(SDLK_m, action) &&
            action == nexus::input::Action::place_widget,
        "M should map to widget placement mode"
    );
    require(
        nexus::platform::SdlInput::keyboard_action(SDLK_x, action) &&
            action == nexus::input::Action::toggle_widget,
        "X should map to widget visibility toggle"
    );
    require(
        !nexus::platform::SdlInput::keyboard_action(SDLK_z, action),
        "unmapped keys should be ignored"
    );
}

void test_gamepad_mapping() {
    SDL_ControllerButtonEvent event{};
    event.button = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    nexus::input::Action action{};
    require(
        nexus::platform::SdlInput::controller_action(event, action) &&
            action == nexus::input::Action::move_left,
        "gamepad D-pad should map to directional navigation"
    );
    event.button = SDL_CONTROLLER_BUTTON_A;
    require(
        nexus::platform::SdlInput::controller_action(event, action) &&
            action == nexus::input::Action::accept,
        "gamepad A should map to accept"
    );
    event.button = SDL_CONTROLLER_BUTTON_GUIDE;
    require(
        nexus::platform::SdlInput::controller_action(event, action) &&
            action == nexus::input::Action::ps_button,
        "gamepad Guide should map to the PS button action"
    );
    event.button = SDL_CONTROLLER_BUTTON_X;
    require(
        nexus::platform::SdlInput::controller_action(event, action) &&
            action == nexus::input::Action::place_widget,
        "gamepad Square should enter widget placement mode"
    );
    event.button = SDL_CONTROLLER_BUTTON_START;
    require(
        nexus::platform::SdlInput::controller_action(event, action) &&
            action == nexus::input::Action::options,
        "gamepad Start should enter widget edit options"
    );
    event.button = SDL_CONTROLLER_BUTTON_Y;
    require(
        nexus::platform::SdlInput::controller_action(event, action) &&
            action == nexus::input::Action::cycle_background,
        "gamepad Y should cycle Welcome Hub backgrounds"
    );
}

} // namespace

int main() {
    test_keyboard_mapping();
    test_gamepad_mapping();
}
