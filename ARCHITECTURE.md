# Architecture

## Current implementation

- `nexus_core`: render/configuration validation, persistent host configuration,
  atomic file replacement, and logging.
- `nexus_guest`: an independent x86-64 interpreter, guest address space,
  limited ELF64 loader, synchronous execution runtime, and initial
  process/thread abstractions. It is not linked into or required by the shell.
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
- `nexus_guest_tests`: original machine-code/ELF fixtures for CPU, memory,
  loader, executor, process lifecycle, error reporting, and runtime events.

## Guest execution foundation

`nexus_guest` separates guest virtual addresses from host pointers. Its
`AddressSpace` maps page-aligned regions and checks read, write, and execute
permissions on every access. The ELF loader uses a separate initialization
operation to populate newly mapped pages before guest execution.

The x86-64 interpreter is an instruction-at-a-time C++ implementation, not a
host JIT. It reports structured faults for unmapped or forbidden accesses,
unsupported encodings, and exhausted instruction budgets. Supported
instructions are intentionally limited:

- `NOP` (`90`).
- `MOV`: `B8+rd`, `89`, `8B`, `C7 /0`; and `LEA` (`8D`).
- `ADD`: `01`, `03`, `05`, `81/83 /0`; `OR` (`81/83 /1`); `AND`
  (`81/83 /4`) and `TEST` (`85`).
- `SUB`: `29`, `2B`, `2D`, `81/83 /5`; `XOR`: `31`, `33`, `35`,
  `81/83 /6`; `CMP`: `39`, `3B`, `3D`, `81/83 /7`.
- `INC` / `DEC` (`FF /0`, `FF /1`).
- Short conditional branches (`70`-`7F`), near conditional branches
  (`0F 80`-`0F 8F`), direct `JMP` (`E9`, `EB`), and direct `CALL` (`E8`).
- Indirect `CALL` / `JMP` and `PUSH r/m64` (`FF /2`, `/4`, `/6`).
- `PUSH` / `POP` general registers (`50`-`5F`), `PUSH imm32` (`68`),
  `PUSH imm8` (`6A`), and near `RET` (`C3`).

Register and memory forms support 32-bit and 64-bit operands, REX register
extensions, ModRM/SIB addressing, disp8/disp32, and RIP-relative addressing.
Only CF, PF, ZF, SF, and OF are maintained for supported arithmetic and
conditional branches. Byte/16-bit operations, string instructions, x87,
SIMD, atomics, interrupts, syscalls, and privileged instructions are not
implemented. Unsupported instructions stop execution with an explicit error.

The ELF parser accepts little-endian ELF64 for `EM_X86_64` and `ET_EXEC`,
validates program and section table bounds, exposes section names/metadata,
and validates that the entry point belongs to an executable `PT_LOAD`.
Loading is limited to non-overlapping page mappings of `PT_LOAD` segments.
`PT_INTERP`, `PT_DYNAMIC`, and allocated `REL`/`RELA` relocations are rejected;
there is no dynamic linker, relocation engine, ELF32, `ET_DYN`, SELF, or PRX
support. The test executable is an original static ELF fixture generated
inside the test and contains no Sony data.

Relocation research follows the public [x86-64 psABI](https://gitlab.com/x86-psABIs/x86-64-ABI)
and [ELF generic program-header specification](https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.pheader.html):
a future relocation component must inspect relocation records and symbols
separately from segment mapping, validate each relocation target against
mapped guest memory, and apply only explicitly implemented relocation types.
No relocation type is currently applied; dynamic symbol lookup, PLT/GOT
resolution, and rebasing are not inferred or emulated.

`GuestExecutor` creates a bounded address space and stack, enters at `e_entry`,
and treats a top-level `RET` to a NEXUS sentinel as clean termination; RAX is
reported as the exit value. It does not provide a Linux or PS5 process ABI,
syscalls, filesystem, or dynamic loader. The kernel foundation currently
tracks one synchronous main thread per process. `KernelMutex` is host-side
synchronization, not a guest mutex implementation.

The runtime publishes application metadata, process status, exit results,
errors, and return-to-shell events through an SDL-independent interface.
It is not yet connected to `SystemShell`; local sample metadata must be marked
as mock data.

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

Guest CPU, Memory, ELF Loader, and Kernel foundations now exist with the
limits above. Filesystem, guest Graphics/Audio/Input services, PS5 APIs, and
shell-to-emulator integration remain unimplemented. The shell is never
represented as the original Sony `SceShellUI.elf`.
