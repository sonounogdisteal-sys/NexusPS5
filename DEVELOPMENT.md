# Development

## Requirements

- CMake 3.20 or newer and a C++20-capable compiler.
- SDL2 2.0.18+ development package and SDL2_ttf 2.0.18+ development package.
- A system TrueType font. On Linux, DejaVu Sans is the tested default; set
  `NEXUS_FONT_PATH` to an alternate font file when needed.
- Vulkan SDK and a physical GPU are not required.

On Ubuntu/Codespaces:

```sh
sudo apt-get update
sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core
```

## Configure, build, and test

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The guest library and its tests are compiled with `-Wall -Wextra -Wpedantic
-Werror`. For the same strict flags on every target, configure with:

```sh
cmake -S . -B build-strict -DBUILD_TESTING=ON \
  -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build build-strict --parallel 2
ctest --test-dir build-strict --output-on-failure
```

The CI smoke test uses SDL's dummy video/audio drivers and software renderer.
It creates an SDL window and renders Home, a selected Home entry, Game Hub,
Game Library, Settings, and Control Center, writing BMP captures into the
build directory. This verifies SDL window creation and software rendering,
not an interactive desktop session.

## Run the shell

```sh
./build/nexus_system_shell_app
```

Use arrow keys or a mapped controller D-pad to navigate; Enter/controller A
selects; Escape/controller B goes back; P/controller Guide opens the Control
Center; Q exits. On Welcome Hub, O/controller Start enters edit mode,
controller A toggles a widget, controller X (Square on PlayStation-style
layouts) starts widget movement, and controller L1/R1 resize it. Arrow keys
move the selected widget while moving; M and keyboard brackets provide
keyboard equivalents. B/controller Y selects among original local
backgrounds. Settings can change host fullscreen and window size, edit a local
profile label, and enable contrast. Other categories are labeled unsupported.
Config and layout files default to the current directory and can be overridden
with `--config PATH` and `--layout PATH`.

The shell uses 1280x720 logical coordinates; the host window can be resized
or switched to fullscreen.

## Interactive desktop test procedure

Run `./build/nexus_system_shell_app` from a graphical desktop session. The
automated dummy-driver smoke test does not substitute for these manual checks:

1. Confirm a visible window opens and the Home background, tiles, and text
   render. Resize the window and confirm the logical layout remains legible.
2. Navigate with arrow keys, hover/click a Home tile, and open Settings,
   Game Library, Game Hub, and Welcome Hub. Confirm Escape returns to Home.
3. Open and close Control Center with P. Verify focus movement, the slide
   transition, and that Home remains available underneath.
4. In Welcome Hub, enter edit mode, toggle a widget, move it with Square
   (controller X) plus the D-pad, place it with Cross (controller A), resize
   with L1/R1, change the local background, exit, and restart to verify layout
   persistence.
5. Connect a controller before launch and while the application is open.
   Verify detection/navigation. Check Guide/PS-button mapping only on a
   controller whose SDL mapping exposes `SDL_CONTROLLER_BUTTON_GUIDE`.
6. In Settings, change fullscreen and render size and verify the host window
   responds; change high contrast/profile label and verify they persist.
   Press Q or close the window and confirm clean exit.

No physical controller or interactive desktop validation is claimed from the
headless CI run. The current SDL path uses mapped game-controller buttons; it
does not guarantee that every controller exposes a PS/Guide button.

## Guest execution tests

`nexus_guest_tests` creates an original x86-64 ELF64 `ET_EXEC` fixture
in-memory; it does not depend on host assemblers, proprietary files, firmware,
or downloaded binaries. The guest instructions perform integer arithmetic,
compare and branch, call/return through a stack, store and reload memory, and
return an exit value. The test target also checks page permissions, mapping
bounds, malformed ELF handling, section inspection, instruction limits,
process/thread transitions, synchronization, and shell-facing runtime events.

Run the guest tests alone with:

```sh
ctest --test-dir build -R nexus_guest_tests --output-on-failure
```

`nexus_guest` is a library foundation, not a command-line application loader.
It accepts only the ELF64 x86-64 `ET_EXEC` subset and instruction encodings
listed in [ARCHITECTURE.md](ARCHITECTURE.md). A top-level guest `RET` to the
NEXUS return sentinel ends the test program; Linux/PS5 syscalls, ELF dynamic
linking, relocations, SELF/PRX, and PS5 compatibility are not provided.

## Current behavior

`nexus --help` and `nexus --version` remain the emulator frontend CLI. The
separate `nexus_system_shell_app` runs an independent native UI prototype.
Its sample entries do not launch executable content. Game execution, media
playback, PlayStation Network, trophies, game activity services, battery
telemetry, and the console-only settings services are unavailable.

## Engineering constraints

- Keep the shell distinct from the emulator frontend and native from a
  hypothetical emulated guest.
- Record unknown behavior as unknown; cite research for UI decisions.
- Do not use proprietary Sony binaries/assets or claim original-shell execution.
- Do not claim compatibility without reproducible tests.
- Keep headless CI independent of Vulkan and physical GPU access.
