#include "nexus/core/config.hpp"
#include "nexus/platform/sdl_canvas.hpp"
#include "nexus/platform/sdl_input.hpp"
#include "nexus/shell/system_shell.hpp"

#include <SDL.h>

#include <iostream>
#include <string>
#include <string_view>

namespace {

struct Arguments {
    bool smoke_test = false;
    std::string screenshot_path;
};

bool parse_arguments(int argc, char** argv, Arguments& arguments) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--help") {
            std::cout << "Usage: nexus_system_shell_app [--smoke-test] [--screenshot PATH]\n";
            return false;
        }
        if (argument == "--smoke-test") {
            arguments.smoke_test = true;
        } else if (argument == "--screenshot" && index + 1 < argc) {
            arguments.screenshot_path = argv[++index];
        } else {
            std::cerr << "Unsupported or incomplete argument: " << argument << '\n';
            return false;
        }
    }
    if (!arguments.screenshot_path.empty() && !arguments.smoke_test) {
        std::cerr << "--screenshot can only be used with --smoke-test.\n";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    Arguments arguments;
    if (!parse_arguments(argc, argv, arguments)) {
        return argc == 2 && std::string_view(argv[1]) == "--help" ? 0 : 2;
    }

    const nexus::core::Config config;
    if (const auto error = config.validate()) {
        std::cerr << "Invalid configuration: " << *error << '\n';
        return 1;
    }

    nexus::platform::SdlCanvas canvas;
    std::string error;
    if (!canvas.initialize(
            "NEXUS PS5 - Independent System Shell",
            config.render_width,
            config.render_height,
            arguments.smoke_test,
            arguments.smoke_test,
            error
        )) {
        std::cerr << error << '\n';
        return 1;
    }
    if (!error.empty()) {
        std::cerr << "Warning: " << error << '\n';
    }

    nexus::shell::SystemShell shell;
    if (arguments.smoke_test) {
        shell.render(canvas);
        if (!canvas.last_error().empty()) {
            std::cerr << canvas.last_error() << '\n';
            return 1;
        }
        const std::string home_path = arguments.screenshot_path.empty()
            ? "nexus-shell-home.bmp"
            : arguments.screenshot_path;
        if (!canvas.save_screenshot(home_path, error)) {
            std::cerr << error << '\n';
            return 1;
        }

        shell.handle(nexus::input::Action::move_right);
        shell.render(canvas);
        const std::string selected_path = home_path + ".selected-app.bmp";
        if (!canvas.last_error().empty()) {
            std::cerr << canvas.last_error() << '\n';
            return 1;
        }
        if (!canvas.save_screenshot(selected_path, error)) {
            std::cerr << error << '\n';
            return 1;
        }

        shell.handle(nexus::input::Action::ps_button);
        shell.render(canvas);
        const std::string overlay_path = home_path + ".control-center.bmp";
        if (!canvas.last_error().empty()) {
            std::cerr << canvas.last_error() << '\n';
            return 1;
        }
        if (!canvas.save_screenshot(overlay_path, error)) {
            std::cerr << error << '\n';
            return 1;
        }
        std::cout << "Window/render smoke test passed; screenshots: "
                  << home_path << ", " << selected_path << ", and "
                  << overlay_path << '\n';
        return 0;
    }

    nexus::platform::SdlInput input;
    input.open_connected_gamepads();
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                nexus::input::Action action;
                if (nexus::platform::SdlInput::keyboard_action(event.key.keysym.sym, action)) {
                    if (action == nexus::input::Action::quit) {
                        running = false;
                    } else {
                        shell.handle(action);
                    }
                }
            } else if (event.type == SDL_CONTROLLERDEVICEADDED ||
                       event.type == SDL_CONTROLLERDEVICEREMOVED) {
                input.handle_device_event(event);
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                nexus::input::Action action;
                if (nexus::platform::SdlInput::controller_action(event.cbutton, action)) {
                    shell.handle(action);
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                int x = event.motion.x;
                int y = event.motion.y;
                canvas.window_to_logical(x, y);
                shell.mouse_move(x, y);
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                int x = event.button.x;
                int y = event.button.y;
                canvas.window_to_logical(x, y);
                shell.mouse_click(x, y);
            }
        }

        if (!running) {
            break;
        }
        shell.render(canvas);
        if (!canvas.last_error().empty()) {
            std::cerr << canvas.last_error() << '\n';
            return 1;
        }
        canvas.present();
        SDL_Delay(16);
    }
    return 0;
}
