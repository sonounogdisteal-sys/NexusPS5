# Roadmap

Status reflects code in this repository; planned items are not implemented.

1. **Foundation — in progress:** C++20/CMake, core logging, basic configuration
   validation, CLI entry point, and automated tests.
2. **Public research:** document evidence and confidence for the PS5 shell and
   `SceShellUI.elf`; research the RPCSX project for technical context only.
3. **Independent shell prototype:** only after research, implement and test
   navigation and one screen at a time; clearly distinguish local sample data
   from online services.
4. **Core emulation primitives:** define guest memory and executable parsing
   based on legally usable homebrew test inputs.
5. **Compatibility services:** add independently tested process, thread, and
   synchronization behavior.
6. **Graphics, audio, and input:** add host abstractions and backends that can
   build without a physical GPU.
7. **Integration and validation:** integrate the shell runtime, publish
   measured compatibility results, and compare screenshots against cited,
   legally usable references.

This sequence is provisional. No schedule or compatibility claim is implied.
DRM, signature, encryption, or console security circumvention is out of scope.
