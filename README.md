# NEXUS PS5

NEXUS PS5 is an independent emulator research and development project. It is
not affiliated with Sony Interactive Entertainment. The project does not run
Sony's `SceShellUI.elf`, include proprietary firmware or assets, or claim to
emulate PS5 applications.

The native graphical shell now includes a researched Welcome Hub-style home,
a local Game Library and Game Hub, functional host Settings, editable local
widgets, keyboard/mouse/controller navigation, and a separate animated
Control Center. These are independently written UI prototypes; local entries
do not launch guest applications. The emulator frontend (`nexus`) remains
separate from the shell application (`nexus_system_shell_app`). A new
independent guest module executes an original static x86-64 ELF test fixture
using a deliberately limited interpreter; it is not a PS5 ABI or
game-compatibility implementation.

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

Navigate with arrow keys or a mapped controller D-pad. Enter/controller A
selects, Escape/controller B goes back, P/controller Guide opens the Control
Center, and Q exits. On the Welcome Hub, O/controller Start enters widget edit
mode, X toggles widget visibility, M/controller X moves a widget, brackets or
controller shoulders resize it, and B/controller Y changes the local
background. These mappings are NEXUS host actions, not PlayStation firmware
input.

The shell reads `nexusps5.conf` and `nexus-welcome.layout` in the current
directory by default. Override them with `--config PATH` and `--layout PATH`.
Settings persist host fullscreen, host window size, local profile label, and
high-contrast preference. Other settings explicitly report as unsupported.
Widget layout and background selection are local-only; there is no PlayStation
Network integration.

The headless screenshot test writes Home, selected Home, Game Hub, Game
Library, Settings, and Control Center captures under `build/`. See
[DEVELOPMENT.md](DEVELOPMENT.md) for a desktop test procedure and known
hardware-validation limits.

The `nexus_guest` library documents its supported instruction subset, memory
model, ELF limits, and process/runtime boundary in
[ARCHITECTURE.md](ARCHITECTURE.md). It supports only little-endian x86-64 ELF64
`ET_EXEC` test binaries with static `PT_LOAD` segments; dynamic linking,
relocations, PS5 SELF/PRX, firmware, system calls, and commercial games are
not supported. See [COMPATIBILITY.md](COMPATIBILITY.md) for the status
registry.

See [ARCHITECTURE.md](ARCHITECTURE.md), [ROADMAP.md](ROADMAP.md),
[COMPATIBILITY.md](COMPATIBILITY.md), [SCE_SHELL_RESEARCH.md](SCE_SHELL_RESEARCH.md), and
[DEVELOPMENT.md](DEVELOPMENT.md) for scope and known limitations.
