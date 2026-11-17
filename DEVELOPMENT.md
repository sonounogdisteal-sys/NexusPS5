# Development

## Requirements

- CMake 3.20 or newer.
- A C++20-capable compiler.
- No third-party dependencies for the current core milestone.

## Configure, build, and test

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The build is intentionally independent of Vulkan and physical GPU access.
Those are future decisions and must not become required for basic CI builds.

## Current behavior

`nexus --help` and `nexus --version` are supported. The program does not boot a
guest, render a shell, or launch applications. The initial tests cover
configuration dimension validation and log-level filtering.

## Engineering constraints

- Keep the shell distinct from the emulator frontend.
- Record unknown behavior as unknown; cite research for UI decisions.
- Do not use proprietary Sony binaries/assets or claim original-shell execution.
- Do not claim compatibility without reproducible tests.
- Keep the default build usable without GPU passthrough.
