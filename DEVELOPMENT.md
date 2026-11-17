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

The CI smoke test uses SDL's dummy video/audio drivers and software renderer.
It creates a real SDL window, renders the Welcome Hub view, a selected local
sample entry, and the Control Center overlay, and writes BMP captures into the
build directory. This checks window creation and rendering without requiring
GPU passthrough or a desktop session.

## Run the shell

```sh
./build/nexus_system_shell_app
```

Use arrow keys or a mapped controller D-pad to navigate; Enter/A selects;
Escape/B goes back; P/controller Guide toggles the Control Center; Q exits.
The standard SDL game-controller mapping database determines controller
button mapping; unsupported controllers need a mapping. The shell currently
uses fixed 1280x720 logical coordinates.

## Current behavior

`nexus --help` and `nexus --version` remain the emulator frontend CLI. The
separate `nexus_system_shell_app` runs an independent native UI prototype.
Its sample entries do not launch executable content. Play, media playback,
network cards, profile, and system-service actions are explicitly unavailable.

## Engineering constraints

- Keep the shell distinct from the emulator frontend and native from a
  hypothetical emulated guest.
- Record unknown behavior as unknown; cite research for UI decisions.
- Do not use proprietary Sony binaries/assets or claim original-shell execution.
- Do not claim compatibility without reproducible tests.
- Keep headless CI independent of Vulkan and physical GPU access.
