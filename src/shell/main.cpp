#include "nexus/core/config.hpp"
#include "nexus/platform/sdl_canvas.hpp"
#include "nexus/platform/sdl_input.hpp"
#include "nexus/shell/system_shell.hpp"

#include <SDL.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

struct Arguments {
    bool smoke_test = false;
    std::string screenshot_path;
    std::filesystem::path config_path = "nexusps5.conf";
    std::filesystem::path layout_path = "nexus-welcome.layout";
};

bool parse_arguments(int argc, char** argv, Arguments& arguments) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--help") {
            std::cout << "Usage: nexus_system_shell_app [--smoke-test] [--screenshot PATH] "
                         "[--config PATH] [--layout PATH]\n";
            return false;
        }
        if (argument == "--smoke-test") {
            arguments.smoke_test = true;
        } else if (argument == "--screenshot" && index + 1 < argc) {
            arguments.screenshot_path = argv[++index];
        } else if (argument == "--config" && index + 1 < argc) {
            arguments.config_path = argv[++index];
        } else if (argument == "--layout" && index + 1 < argc) {
            arguments.layout_path = argv[++index];
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

bool apply_window_update(
    nexus::shell::SystemShell& shell,
    nexus::platform::SdlCanvas& canvas
) {
    const auto settings = shell.take_window_update();
    if (!settings.has_value()) {
        return true;
    }

    SDL_Window* window = canvas.window();
    const auto set_window_size = [&]() {
        SDL_SetWindowSize(window, settings->width, settings->height);
    };
    if (!settings->fullscreen) {
        if (SDL_SetWindowFullscreen(window, 0) != 0) {
            shell.report_system_error(std::string("Could not exit fullscreen: ") + SDL_GetError());
            return false;
        }
        set_window_size();
    } else {
        set_window_size();
        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
            shell.report_system_error(std::string("Could not enter fullscreen: ") + SDL_GetError());
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    Arguments arguments;
    if (!parse_arguments(argc, argv, arguments)) {
        return argc == 2 && std::string_view(argv[1]) == "--help" ? 0 : 2;
    }

    nexus::core::Config config;
    std::string error;
    if (!nexus::core::Config::load_from_file(arguments.config_path, config, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    if (const auto validation_error = config.validate()) {
        std::cerr << "Invalid configuration: " << *validation_error << '\n';
        return 1;
    }

    nexus::platform::SdlCanvas canvas;
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
    if (config.fullscreen &&
        SDL_SetWindowFullscreen(canvas.window(), SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
        std::cerr << "Could not enter configured fullscreen mode: " << SDL_GetError() << '\n';
        return 1;
    }

    nexus::shell::WelcomeHub welcome_hub(arguments.layout_path);
    if (!welcome_hub.load(error)) {
        std::cerr << error << '\n';
        return 1;
    }
    nexus::shell::SystemShell shell(config, arguments.config_path, std::move(welcome_hub));
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
        shell.update(0.25F);
        shell.render(canvas);
        const std::string selected_home_path = home_path + ".selected-home.bmp";
        if (!canvas.save_screenshot(selected_home_path, error)) {
            std::cerr << error << '\n';
            return 1;
        }

        shell.handle(nexus::input::Action::accept);
        shell.render(canvas);
        const std::string game_hub_path = home_path + ".game-hub.bmp";
        if (!canvas.last_error().empty()) {
            std::cerr << canvas.last_error() << '\n';
            return 1;
        }
        if (!canvas.save_screenshot(game_hub_path, error)) {
            std::cerr << error << '\n';
            return 1;
        }

        shell.handle(nexus::input::Action::back);
        shell.handle(nexus::input::Action::move_up);
        shell.handle(nexus::input::Action::move_right);
        shell.handle(nexus::input::Action::accept);
        shell.render(canvas);
        const std::string library_path = home_path + ".game-library.bmp";
        if (!canvas.save_screenshot(library_path, error)) {
            std::cerr << error << '\n';
            return 1;
        }

        shell.handle(nexus::input::Action::back);
        shell.handle(nexus::input::Action::move_right);
        shell.handle(nexus::input::Action::accept);
        shell.render(canvas);
        const std::string settings_path = home_path + ".settings.bmp";
        if (!canvas.last_error().empty()) {
            std::cerr << canvas.last_error() << '\n';
            return 1;
        }
        if (!canvas.save_screenshot(settings_path, error)) {
            std::cerr << error << '\n';
            return 1;
        }

        shell.handle(nexus::input::Action::back);
        shell.handle(nexus::input::Action::ps_button);
        shell.update(0.2F);
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
                  << home_path << ", " << selected_home_path << ", " << game_hub_path
                  << ", " << library_path << ", " << settings_path
                  << ", and " << overlay_path << '\n';
        return 0;
    }

    nexus::platform::SdlInput input;
    input.open_connected_gamepads();
    bool running = true;
    const Uint64 counter_frequency = SDL_GetPerformanceFrequency();
    Uint64 previous_counter = SDL_GetPerformanceCounter();
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
                        static_cast<void>(apply_window_update(shell, canvas));
                    }
                }
            } else if (event.type == SDL_CONTROLLERDEVICEADDED ||
                       event.type == SDL_CONTROLLERDEVICEREMOVED) {
                input.handle_device_event(event);
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                nexus::input::Action action;
                if (nexus::platform::SdlInput::controller_action(event.cbutton, action)) {
                    shell.handle(action);
                    static_cast<void>(apply_window_update(shell, canvas));
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
                static_cast<void>(apply_window_update(shell, canvas));
            }
        }

        if (!running) {
            break;
        }
        const Uint64 current_counter = SDL_GetPerformanceCounter();
        const float delta_seconds = counter_frequency == 0
            ? 0.0F
            : static_cast<float>(current_counter - previous_counter) /
                static_cast<float>(counter_frequency);
        previous_counter = current_counter;
        shell.update(delta_seconds);
        shell.render(canvas);
        if (!canvas.last_error().empty()) {
            std::cerr << canvas.last_error() << '\n';
            return 1;
        }
        canvas.present();
        SDL_Delay(8);
    }
    return 0;
}
