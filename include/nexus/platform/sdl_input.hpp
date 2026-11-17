#pragma once

#include "nexus/input/action.hpp"

#include <SDL.h>

#include <vector>

namespace nexus::platform {

class SdlInput {
public:
    SdlInput() = default;
    ~SdlInput();

    SdlInput(const SdlInput&) = delete;
    SdlInput& operator=(const SdlInput&) = delete;

    void open_connected_gamepads();
    void handle_device_event(const SDL_Event& event);
    [[nodiscard]] static bool keyboard_action(SDL_Keycode key, input::Action& action);
    [[nodiscard]] static bool controller_action(
        const SDL_ControllerButtonEvent& event,
        input::Action& action
    );

private:
    void close_gamepad(SDL_JoystickID instance_id);
    std::vector<SDL_GameController*> gamepads_;
};

} // namespace nexus::platform
