# Architecture

## Current implementation

The CMake project currently builds:

- `nexus_core`: render configuration validation and logging.
- `nexus`: a minimal command-line entry point, not an emulator or shell.
- `nexus_core_tests`: dependency-free tests for the core utilities.

Configuration currently exists only as an in-memory value object; there is no
configuration file format or persistent settings yet.

## Intended boundaries

These are planned boundaries, not implemented subsystems:

- `Core`: shared configuration, logging, and common types.
- `CPU`, `Memory`, `Kernel`, `Loader`, `Filesystem`: independent guest
  execution and system compatibility.
- `Graphics`, `Audio`, `Input`: host device abstractions and guest-facing
  translation layers.
- `SystemShell`: independently written shell application communicating with
  emulator services.
- `Frontend`: emulator configuration and library management, distinct from
  the guest shell.
- `Debug`: diagnostics and test facilities.

The independent shell must never be represented as the original Sony
`SceShellUI.elf`. Guest execution and shell functionality remain blocked until
their interfaces and independently testable runtime components are designed.
