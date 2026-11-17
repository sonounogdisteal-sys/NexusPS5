# Roadmap

Status reflects code in this repository; planned items are not implemented.

1. **Foundation — complete:** C++20/CMake, logging, configuration validation,
   CLI, automated tests, and CI.
2. **Graphics and shell foundation — prototype complete:** SDL2 window/render
   loop, software headless path, SDL_ttf text, keyboard/mouse/mapped-gamepad
   input, reusable UI components/focus management, Home screen structure, and
   a separate Control Center overlay. Exact visual fidelity and Vulkan are not
   complete.
3. **PS5 shell research — in progress:** expand version/region-specific
   evidence and verify exact Home/Control Center geometry and behavior against
   public official screenshots or demonstrations. Unknowns remain documented.
4. **Shell reconstruction — ongoing:** iterate on measured visual differences;
   add documented screens only when backed by research and tested local data.
5. **Core emulation primitives:** define guest memory and executable parsing
   from legally usable homebrew test inputs; do not claim game execution.
6. **Compatibility services:** add independently tested process, thread, and
   synchronization behavior.
7. **Guest graphics, audio, and input:** design backend abstractions, then add
   Vulkan as an optional host backend. Current shell drawing is native SDL2.
8. **Integration and validation:** integrate a shell runtime with actual
   emulator services only after those interfaces exist; publish measured
   compatibility results.

This sequence is provisional. No schedule or compatibility claim is implied.
DRM, signature, encryption, or console security circumvention is out of scope.
