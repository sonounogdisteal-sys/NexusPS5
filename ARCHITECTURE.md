# Architecture

## Current implementation

- `nexus_core`: render configuration validation and logging.
- `nexus`: CLI for the emulator frontend; it does not boot the shell.
- `nexus_ui`: renderer-independent components, scenes, and directional focus.
- `nexus_sdl`: SDL2 window, software/accelerated 2D renderer, text, and
  screenshot adapter.
- `nexus_system_shell`: independent Home screen state and a separate Control
  Center overlay.
- `nexus_system_shell_app`: native host process used to run the shell prototype.
- `nexus_core_tests`, `nexus_ui_tests`, and
  `nexus_shell_window_smoke`: core/UI tests and a headless SDL window/render
  test.

### Rendering and input

SDL2 owns the native window, event loop, 2D renderer, and normalized gamepad
events. SDL_ttf renders a system font without bundling a font asset. The SDL
renderer may use the platform's accelerated 2D driver, with an explicit
software-renderer fallback for headless environments.

Rendering is behind the `ui::Canvas` abstraction. A future Vulkan backend can
use SDL's platform-specific Vulkan instance extensions and surface creation;
this milestone does not create a Vulkan instance or render through Vulkan.
The UI uses a fixed 1280x720 logical coordinate space scaled by SDL.

`ui::Scene` owns reusable components and a `FocusManager`. Focus moves among
explicitly positioned targets without implicit wrapping; mouse motion/clicks
use component bounds. `input::Action` separates shell navigation from SDL
key/controller codes.

### Shell and frontend boundary

The emulator frontend and shell are distinct executables/modules. `nexus`
remains the small frontend CLI. `nexus_system_shell_app` hosts
`nexus::shell::SystemShell` natively for UI development. This is not yet a
guest process and is not launched through an emulated graphics/runtime
interface.

The shell shows local, explicitly non-executable sample entries. The
Control Center overlay has its own scene and focus state. Documented control
names and the PS-button activation are represented, but controls requiring
kernel, network, audio, or game services report as unimplemented.

CPU, Memory, Kernel, Loader, Filesystem, guest Graphics/Audio/Input services,
and shell-to-emulator integration remain unimplemented. The shell is never
represented as the original Sony `SceShellUI.elf`.
