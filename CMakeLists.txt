cmake_minimum_required(VERSION 3.20)

project(NexusPS5 VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
find_package(SDL2 2.0.18 CONFIG REQUIRED)
find_package(SDL2_ttf 2.0.18 CONFIG REQUIRED)

add_library(nexus_core
    src/core/config.cpp
    src/core/logger.cpp
)
target_include_directories(nexus_core PUBLIC include)
target_compile_features(nexus_core PUBLIC cxx_std_20)

add_library(nexus_ui
    src/ui/component.cpp
    src/ui/focus_manager.cpp
)
target_include_directories(nexus_ui PUBLIC include)
target_compile_features(nexus_ui PUBLIC cxx_std_20)

add_library(nexus_system_shell
    src/shell/control_center.cpp
    src/shell/system_shell.cpp
)
target_include_directories(nexus_system_shell PUBLIC include)
target_compile_features(nexus_system_shell PUBLIC cxx_std_20)
target_link_libraries(nexus_system_shell PUBLIC nexus_ui)

add_library(nexus_sdl
    src/platform/sdl_canvas.cpp
    src/platform/sdl_input.cpp
)
target_include_directories(nexus_sdl PUBLIC include)
target_compile_features(nexus_sdl PUBLIC cxx_std_20)
target_link_libraries(nexus_sdl PUBLIC nexus_ui SDL2::SDL2 SDL2_ttf::SDL2_ttf)

add_executable(nexus src/frontend/main.cpp)
target_link_libraries(nexus PRIVATE nexus_core)

add_executable(nexus_system_shell_app
    src/shell/main.cpp
)
target_link_libraries(nexus_system_shell_app PRIVATE
    nexus_core
    nexus_sdl
    nexus_system_shell
)

if(BUILD_TESTING)
    add_executable(nexus_core_tests tests/core_tests.cpp)
    target_link_libraries(nexus_core_tests PRIVATE nexus_core)
    add_test(NAME nexus_core_tests COMMAND nexus_core_tests)

    add_executable(nexus_ui_tests tests/ui_tests.cpp)
    target_link_libraries(nexus_ui_tests PRIVATE nexus_system_shell)
    add_test(NAME nexus_ui_tests COMMAND nexus_ui_tests)

    add_executable(nexus_input_tests tests/input_tests.cpp)
    target_link_libraries(nexus_input_tests PRIVATE nexus_sdl)
    add_test(NAME nexus_input_tests COMMAND nexus_input_tests)

    add_test(
        NAME nexus_shell_window_smoke
        COMMAND nexus_system_shell_app --smoke-test --screenshot
            "${CMAKE_CURRENT_BINARY_DIR}/nexus-shell-smoke.bmp"
    )
    set_tests_properties(nexus_shell_window_smoke PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
    )
endif()
