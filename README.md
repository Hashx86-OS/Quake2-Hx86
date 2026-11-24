# Quake2-HX86

Port of the Quake II engine to the [Hashx86 OS](https://github.com/Hashx86-OS/Hashx86-OS) kernel.

## Status

Software renderer, keyboard + mouse input, no sound. Runs single-player. The
app can download and install id Software's original Quake II 3.14 shareware
demo data on first launch.

## Prerequisites

- GCC/g++ with multilib support (for `-m32`)
- GNU `ld` (for `-melf_i386`)
- `make`
- A Hashx86-OS checkout for `libhx86` and `libmbedtls`

## Build

```bash
make KERNEL_DIR=/path/to/Hashx86-OS
```

Output: `build/Quake2.bin`

The build defaults `KERNEL_DIR` to `../Hashx86-OS`.

## Game data

The application looks for `/baseq2/pak0.pak`. If it is missing, the app shows a
download dialog and installs:

```text
/baseq2/pak0.pak
/baseq2/players/...
/baseq2/licenses/QUAKE2LIC.TXT
```

The original `q2-314-demo-x86.exe` archive is kept under
`/baseq2/shareware/`. It is downloaded from gamers.org, verified against its
pinned SHA-256, and extracted locally. Hashx86 does not host or redistribute
the demo archive.

Retail Quake II data is proprietary and must be supplied by the user from a
legal installation. Place its `pak0.pak` at `/baseq2/pak0.pak`.

## Install to Hashx86 OS

The App Store installs the binary as `/Apps/quake2/Quake2.bin`; the engine's
base directory remains `/baseq2`. To install manually, copy
`build/Quake2.bin` to `/Apps/quake2/Quake2.bin` (or `/Hashx86/apps/`) on the
disk image.

## Files

| Path | Description |
|---|---|
| `Quake2/engine/client/` | Client code (rendering, input, sound) |
| `Quake2/engine/server/` | Server code (networking, physics, AI) |
| `Quake2/engine/qcommon/` | Common code (filesystem, cvars, commands) |
| `Quake2/engine/ref_soft/` | Software renderer |
| `Quake2/engine/game/` | Game logic (statically linked) |
| `Quake2/engine/null/` | Null sound/CD drivers |
| `Quake2/prog.cpp` | Entry point and shareware installer UI |
| `Quake2/data/Archive.*` | Quake II demo download and extraction |
| `Quake2/data/HttpClient.*` | HTTPS client (mbedTLS) |
| `Quake2/data/StoreUtil.*` | Small filesystem/string helpers |
| `Quake2/compat.c` | C standard library stubs + syscall wrappers |
| `Quake2/sys_hx86.c` | System interface (file I/O, timing) |
| `Quake2/vid_hx86.c` | Video output + input (VESA framebuffer) |
| `Quake2/q_shhx86.c` | Memory allocation |
| `third_party/miniz/` | ZIP reader used for the demo archive |
| `Quake2/linker.ld` | Linker script for Hashx86 OS binaries |

## License

- **Engine code**: GPL v2 or later — Copyright (C) 1997-2001 Id Software, Inc.
- **Port code** (`prog.cpp`, `compat.c`, `sys_hx86.c`, `vid_hx86.c`, `q_shhx86.c`, `data/`, `Makefile`, `linker.ld`): GPL v2 or later
- **Quake II demo data**: id Software Limited Use Software Demo License Agreement (1998); downloaded by the app, not bundled
- **Retail game data** (`pak0.pak`): Proprietary — not included
