# NEXUS PS5

NEXUS PS5 is an independent emulator research and development project. It is
not affiliated with Sony Interactive Entertainment. The project does not run
Sony's `SceShellUI.elf`, does not include proprietary firmware or assets, and
does not currently emulate PS5 applications.

The initial C++20 foundation provides a small command-line entry point, typed
render configuration validation, and a threshold-based logger. No shell UI,
guest runtime, graphics backend, or PS5 compatibility is implemented yet.

## Build and test

Requires CMake 3.20+ and a C++20 compiler. The initial core has no third-party
runtime dependencies.

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/nexus --help
```

See [ARCHITECTURE.md](ARCHITECTURE.md), [ROADMAP.md](ROADMAP.md), and
[DEVELOPMENT.md](DEVELOPMENT.md) for current scope and next steps.
