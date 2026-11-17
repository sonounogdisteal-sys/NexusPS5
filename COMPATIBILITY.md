# Compatibility registry

This registry distinguishes NEXUS-hosted test behavior from PlayStation
compatibility. No PS5 firmware, SELF, PRX, retail application, or game has
been loaded or executed. "Not assessed" means there is no verified behavior
to report; it must not be read as compatible or incompatible.

| Area | Status | Evidence and limitations |
|---|---|---|
| C++20 host build and CI | Implemented | See `DEVELOPMENT.md`; this validates NEXUS code only. |
| NEXUS x86-64 interpreter | Partial, independently tested | Implements the documented instruction subset in `ARCHITECTURE.md`; no PS5 workload tested. |
| Guest virtual memory | Partial, independently tested | Page mappings, permissions, and bounded byte access; not a PS5 address-space model. |
| ELF64 `ET_EXEC` | Partial, independently tested | Little-endian x86-64 static test ELF `PT_LOAD` segments; no SELF/PRX or dynamic loader. |
| PS5 executable format / SELF / PRX | Not implemented; compatibility not assessed | No encrypted or signed console executable is parsed or loaded. |
| PS5 CPU behavior and ABI | Not assessed | The NEXUS instruction interpreter is not validated against PS5 applications, extensions, or calling conventions. |
| PS5 kernel calls, process services, synchronization | Not implemented; compatibility not assessed | NEXUS process/thread lifecycle is a host-side scaffold; no PS5 syscall translation. |
| PS5 system software behavior | Not assessed | Sony's public [system software support page](https://www.playstation.com/en-us/support/hardware/ps5/system-software/) documents user-facing update requirements, not a public guest syscall ABI. |
| Guest filesystem, graphics, audio, networking, controllers | Not implemented | No guest service emulation. |
| PlayStation Network, trophies, game compatibility | Not implemented; not assessed | No network or console service integration. |
| NEXUS system shell | Independent host UI prototype | The shell does not run as a guest and is not Sony firmware. |

## Research notes

- The public [RPCSX repository](https://github.com/RPCSX/rpcsx) describes an
  experimental PS4/PS5 emulator and separates kernel-related work from other
  emulator directories. Its public [usage page](https://github.com/RPCSX/rpcsx/blob/master/.github/USAGE.md)
  documents a sample invocation and explicitly names firmware 5.05 for PS4;
  that is not evidence of PS5 application compatibility.
- The retrieved RPCSX landing and usage pages do not provide a reproducible
  per-title PS5 compatibility matrix. No specific PS5 game limitation is
  inferred from them. The repository is used only as architectural and
  project-status reference; no code or implementation was copied.
- Sony's public update support page confirms that system software versions
  and application/content requirements can matter, but does not publish the
  system-call, executable, or kernel interfaces needed to claim compatibility.

Future entries must identify the exact input, NEXUS commit, host environment,
test procedure, observed result, and limits. A successful original test ELF is
not evidence of PS5 software or game compatibility.
