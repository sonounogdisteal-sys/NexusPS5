# Architecture

## Current implementation

- `nexus_core`: render/configuration validation, persistent host configuration,
  atomic file replacement, and logging.
- `nexus`: CLI for the emulator frontend; it does not boot the shell.
- `nexus_ui`: renderer-independent components, scenes, directional focus,
  reusable vector icons, visual tokens, and scalar/color animations.
- `nexus_sdl`: SDL2 window, software/accelerated 2D renderer, text, and
  screenshot adapter.
- `nexus_system_shell`: independent Home, Game Library, Game Hub, Settings,
  and Welcome Hub views; a separate Control Center overlay; and local widget
  layout persistence.
- `nexus_system_shell_app`: native host process used to run the shell prototype.
- `nexus_core_tests`, `nexus_ui_tests`, and
  `nexus_shell_window_smoke`: core/UI tests and a headless SDL window/render
  test.

### Rendering and input

SDL2 owns the native window, event loop, 2D renderer, normalized gamepad
events, and host window changes requested by Settings. SDL_ttf renders a
system font without bundling a font asset. The SDL renderer may use the
platform's accelerated 2D driver, with an explicit software-renderer fallback
for headless environments.

Rendering is behind the `ui::Canvas` abstraction. A future Vulkan backend can
use SDL's platform-specific Vulkan instance extensions and surface creation;
this milestone does not create a Vulkan instance or render through Vulkan.
The UI uses a fixed 1280x720 logical coordinate space scaled by SDL.

`ui::Scene` owns reusable components and a `FocusManager`. Focus moves among
explicitly positioned targets without implicit wrapping; mouse motion/clicks
use component bounds. `input::Action` separates shell navigation from SDL
key/controller codes. Time-based transitions are updated by the native shell
loop, not by components that depend on SDL.

### Shell and frontend boundary

The emulator frontend and shell are distinct executables/modules. `nexus`
remains the small frontend CLI. `nexus_system_shell_app` hosts
`nexus::shell::SystemShell` natively for UI development. This is not yet a
guest process and is not launched through an emulated graphics/runtime
interface.

The Home screen retains a horizontal Games/Media app row, selected-entry
artwork, settings/profile access, and a local Welcome Hub widget area. The
Game Library and Game Hub contain labeled local test data. Settings update
the host window size/fullscreen, local profile label, and contrast preference;
other categories state that their services are unsupported. Welcome Hub
widget visibility, position, size, and local background choice are persisted
separately. No account or network service is connected.

The Control Center is a separate overlay and focus scene. It animates on/off,
supports horizontal navigation and local icon reordering, and retains
contextual sample cards. Controls requiring kernel, network, audio, controller
telemetry, or game services report as unimplemented; hiding controls is not
implemented.

CPU, Memory, Kernel, Loader, Filesystem, guest Graphics/Audio/Input services,
and shell-to-emulator integration remain unimplemented. The shell is never
represented as the original Sony `SceShellUI.elf`.
