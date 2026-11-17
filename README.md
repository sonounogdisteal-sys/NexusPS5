# NEXUS PS5

NEXUS PS5 is an independent emulator research and development project. It is
not affiliated with Sony Interactive Entertainment. The project does not run
Sony's `SceShellUI.elf`, include proprietary firmware or assets, or currently
emulate PS5 applications.

Milestone 2 adds an independently written, native graphical shell prototype:
a Games/Media home view, directional focus, keyboard/mouse/gamepad input, and
a Control Center overlay. All displayed application entries are labeled local
samples; selecting Play does not execute a guest application. The emulator
frontend (`nexus`) remains separate from the shell application
(`nexus_system_shell_app`).

## Build and test

Requires CMake 3.20+, a C++20 compiler, SDL2 2.0.18+, SDL2_ttf 2.0.18+, and a
system font such as DejaVu Sans. No Vulkan SDK or physical GPU is required for
this software renderer milestone.

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/nexus --help
./build/nexus_system_shell_app
```

Navigate with arrow keys or a mapped controller D-pad. Enter/A selects,
Escape/B goes back, and P/controller Guide opens the Control Center. Q exits.
The screenshot smoke test writes home and overlay captures under `build/`.

See [ARCHITECTURE.md](ARCHITECTURE.md), [ROADMAP.md](ROADMAP.md),
[SCE_SHELL_RESEARCH.md](SCE_SHELL_RESEARCH.md), and
[DEVELOPMENT.md](DEVELOPMENT.md) for scope and known limitations.
