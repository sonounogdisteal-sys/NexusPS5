#include "nexus/platform/sdl_input.hpp"

#include <algorithm>

namespace nexus::platform {

SdlInput::~SdlInput() {
    for (SDL_GameController* gamepad : gamepads_) {
        SDL_GameControllerClose(gamepad);
    }
}

void SdlInput::open_connected_gamepads() {
    const int count = SDL_NumJoysticks();
    for (int index = 0; index < count; ++index) {
        if (SDL_IsGameController(index) == SDL_TRUE) {
            SDL_GameController* gamepad = SDL_GameControllerOpen(index);
            if (gamepad != nullptr) {
                gamepads_.push_back(gamepad);
            }
        }
    }
}

void SdlInput::handle_device_event(const SDL_Event& event) {
    if (event.type == SDL_CONTROLLERDEVICEADDED) {
        if (SDL_IsGameController(event.cdevice.which) == SDL_TRUE) {
            SDL_GameController* gamepad = SDL_GameControllerOpen(event.cdevice.which);
            if (gamepad != nullptr) {
                gamepads_.push_back(gamepad);
            }
        }
    } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        close_gamepad(event.cdevice.which);
    }
}

bool SdlInput::keyboard_action(SDL_Keycode key, input::Action& action) {
    switch (key) {
    case SDLK_LEFT: action = input::Action::move_left; return true;
    case SDLK_RIGHT: action = input::Action::move_right; return true;
    case SDLK_UP: action = input::Action::move_up; return true;
    case SDLK_DOWN: action = input::Action::move_down; return true;
    case SDLK_RETURN:
    case SDLK_SPACE: action = input::Action::accept; return true;
    case SDLK_ESCAPE: action = input::Action::back; return true;
    case SDLK_p: action = input::Action::ps_button; return true;
    case SDLK_q: action = input::Action::quit; return true;
    default: return false;
    }
}

bool SdlInput::controller_action(
    const SDL_ControllerButtonEvent& event,
    input::Action& action
) {
    switch (event.button) {
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: action = input::Action::move_left; return true;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: action = input::Action::move_right; return true;
    case SDL_CONTROLLER_BUTTON_DPAD_UP: action = input::Action::move_up; return true;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: action = input::Action::move_down; return true;
    case SDL_CONTROLLER_BUTTON_A: action = input::Action::accept; return true;
    case SDL_CONTROLLER_BUTTON_B: action = input::Action::back; return true;
    case SDL_CONTROLLER_BUTTON_GUIDE: action = input::Action::ps_button; return true;
    case SDL_CONTROLLER_BUTTON_START: action = input::Action::back; return true;
    default: return false;
    }
}

void SdlInput::close_gamepad(SDL_JoystickID instance_id) {
    const auto found = std::find_if(
        gamepads_.begin(),
        gamepads_.end(),
        [instance_id](SDL_GameController* gamepad) {
            return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad)) == instance_id;
        }
    );
    if (found != gamepads_.end()) {
        SDL_GameControllerClose(*found);
        gamepads_.erase(found);
    }
}

} // namespace nexus::platform
