# Roadmap

Status reflects code in this repository; planned items are not implemented.

1. **Foundation — complete:** C++20/CMake, logging, configuration validation,
   CLI, automated tests, and CI.
2. **Graphics and shell foundation — prototype complete:** SDL2 window/render
   loop, software headless path, SDL_ttf text, keyboard/mouse/mapped-gamepad
   input, reusable UI components/focus management, Home screen structure, and
   a separate Control Center overlay. Exact visual fidelity and Vulkan are not
   complete.
3. **PS5 shell research — ongoing:** the visual target is the Home/Welcome Hub
   interface shown in Sony's September 2024 rollout material. The precise
   firmware build is not established by that reference. Later appearance
   features are not mixed into the implementation. Pixel geometry and
   Control Center animation timing remain unverified.
4. **Shell reconstruction — prototype expanded:** separate Game Library,
   Game Hub, Settings, Welcome Hub, and Control Center views; local editable
   widgets and layout persistence; host Settings for window size, fullscreen,
   profile label, and contrast; input mappings and transition timing. Visual
   fidelity and real-desktop/controller validation remain ongoing.
5. **Independent guest execution foundation — initial subset complete:**
   page-permission guest memory, a documented x86-64 interpreter subset,
   static x86-64 ELF64 `ET_EXEC` loading, bounded test-binary execution, and
   initial process/thread/runtime-event abstractions. No PS5 executable or
   game compatibility is claimed; expansion and validation remain future work.
6. **Compatibility services:** add independently tested filesystem,
   process, thread, and synchronization behavior; investigate publicly
   documented interfaces and keep unknown PS5 behavior marked unknown.
7. **Guest graphics, audio, and input:** design backend abstractions, then add
   Vulkan as an optional host backend. Current shell drawing is native SDL2.
8. **Integration and validation:** integrate a shell runtime with actual
   emulator services only after those interfaces exist; publish measured
   compatibility results.

This sequence is provisional. No schedule or compatibility claim is implied.
DRM, signature, encryption, or console security circumvention is out of scope.
