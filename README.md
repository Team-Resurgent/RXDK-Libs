# RXDK-Libs

<p align="center"><b>A from-source, MSVC-free C/C++ runtime and SDK for the original Xbox — picolibc + LLVM libc++, an xAPI, and the D3D8 / D3DX8 / DirectSound / DirectMusic / XGraphics / XMV / XNet / XACT / XOnline libraries, built with Zig</b></p>

<p align="center">
  <a href="https://github.com/Team-Resurgent/RXDK-Libs/blob/main/LICENSE.md"><img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="License: GPL v3"></a>
  <a href="https://discord.gg/VcdSfajQGK"><img src="https://img.shields.io/badge/chat-on%20discord-7289da.svg?logo=discord" alt="Discord"></a>
</p>

<p align="center">
  <a href="https://ko-fi.com/J3J7L5UMN"><img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="ko-fi"></a>
  <a href="https://www.patreon.com/teamresurgent"><img src="https://img.shields.io/badge/Patreon-F96854?style=for-the-badge&logo=patreon&logoColor=white" alt="Patreon"></a>
</p>

Zig-built Xbox C/C++ runtime and SDK for original Xbox devkits — **picolibc** + **LLVM libc++**, an **xAPI**, and the **D3D8 / D3DX8 / DirectSound / XGraphics / XMV / XNet** subsystem libraries, ISO C23 / C++23.

No Visual Studio, MSBuild, `cl.exe`, or Windows SDK is required to build the runtime in this repo. The host-side deploy tools (`imagebld`, `xdvdfs`, Xbox Neighborhood, etc.) and the IDE integrations live in the separate [RXDK-Tools](https://github.com/Team-Resurgent/RXDK-Tools) and RXDK extension repos.

## Origins & attribution

RXDK's low-level runtime (`libc`, `libc++`, `libkernel`) and the entire Zig/Clang build system are original work. The higher-level subsystem libraries — the xAPI and the D3D8, D3DX8, DirectSound, XGraphics, XMV, XNet, XACT and XOnline drivers — are **derived from Microsoft's original Xbox Development Kit source**.

That source has been **recompiled from source against a modern, MSVC-free toolchain** (Zig + Clang, picolibc + LLVM libc++) rather than the original Microsoft compiler, and adapted extensively to build and run under it — a large amount of code had to be reworked for the new ABI, calling conventions and freestanding runtime.

Where the leaked source was incomplete, missing functionality was **recovered by decompiling and disassembling the shipped retail Xbox libraries** — reconstructing the absent functions from the prebuilt `.lib`/binary objects (via symbol recovery and decompilation) — so the libraries match the retail XDK build they target.

"Xbox" and all related names are trademarks of Microsoft Corporation. RXDK is an independent, non-commercial preservation and homebrew project; it is **not affiliated with, authorised by, or endorsed by Microsoft**. It is distributed under the GNU GPL v3 (see [LICENSE.md](LICENSE.md)) on a best-effort basis with no warranty.

## Prerequisites

- [Zig](https://ziglang.org/) **0.16+** (tested with 0.16.0)
- Git submodules: `vendor/picolibc`, `vendor/llvm-project` (sparse checkout for `libcxx` + `libcxxabi`)

```powershell
.\scripts\init-submodules.ps1
```

Or manually:

```powershell
git submodule update --init vendor/picolibc vendor/llvm-project
git -C vendor/llvm-project sparse-checkout init --cone
git -C vendor/llvm-project sparse-checkout set libcxx libcxxabi
```

All library sources are committed in this repo under `libs/`. The xAPI and subsystem libraries (`libxapi`, `libd3d8`, `libd3dx8`, `libdsound`, `libxgraphics`, `libxmv`, `libxnet`, …) build directly from the in-tree sources — no external source checkout is required.

## Layout

```
build.zig / build/         Zig build graph (orchestration + generated headers)
shared/include/            Public distributed headers (xt.h umbrella, xapi.h, xbox.h,
                           xkbd.h, windef/winbase, xboxkrnl/, xbox/, d3d8/dsound/xnet/…)
shared/picolibc/           picolibc C headers (headers-only distribution)
shared/libcxx/             LLVM libc++ headers (headers-only distribution)
libs/libc/                 First-party libc runtime — xbox/ (HAL, crt0, kernel glue) + c23/ gap-fill (→ libc.lib)
libs/libcpp/               libc++ build orchestration over vendored libcxx (→ libcpp.lib)
libs/libkernel/            Xbox kernel import lib, generated from xboxkrnl.def (→ libkernel.lib)
libs/libxbdm/              Xbox debug monitor import lib (→ libxbdm.lib)
libs/libxapi/              xAPI port — k32 + dll + rtl + uuid + USB (→ libxapi.lib)
libs/libd3d8/              Xbox D3D8 (NV2A) graphics driver (→ libd3d8.lib / libd3d8i.lib)
libs/libd3dx8/             D3DX8 helper / utility library (→ libd3dx8.lib)
libs/libxgraphics/         XGraphics swizzle / texture utilities + XFONT (→ libxgraphics.lib)
libs/libdsound/            DirectSound (MCPX APU) audio (→ libdsound.lib)
libs/libxmv/               XMV video decoder (→ libxmv.lib)
libs/libxnet/              Xbox net stack — XNet / winsock (→ libxnet.lib / libxneto.lib)
libs/libxact/              XACT audio engine (→ libxact.lib)
libs/libxonline/           Xbox Live client + UIX (→ libxonline.lib / libuix.lib)
libs/libxvoice/            Xbox Communicator voice (→ libxvoice.lib)
libs/libdmusic/            DirectMusic (→ libdmusic.lib)
vendor/picolibc/           picolibc C library sources (submodule)
vendor/llvm-project/       libc++ / libcxxabi sources (submodule, sparse)
zig-out/lib/               Staged .lib outputs + object response files (per `zig build`)
zig-out/include/           Staged C / C++ / xAPI / subsystem headers (after `zig build`)
dist/                      Redistributable bundle from build.ps1 — lib/{debug,release} + include/ (gitignored)
build.ps1                  Build the redistributable library distribution (Debug + ReleaseSmall)
```

## Build

```powershell
cd D:\Git\RXDK-Libs
.\build.ps1                 # build the dist (Debug + ReleaseSmall) → dist\lib\{debug,release} + dist\include
.\build.ps1 -Clean         # clean the zig cache first, then build the dist from scratch
.\scripts\compile.ps1 -Target libs   # build every library + staged headers into zig-out (no dist packaging)
```

Or invoke `zig build` directly:

```powershell
zig build verify-no-vs    # assert build/*.zig never invokes MSVC toolchain
zig build                 # every library + staged headers into zig-out
zig build libkernel       # libkernel.lib (kernel import lib) only
zig build libxapi         # libxapi.lib only
zig build libd3d8         # a single subsystem lib (libd3d8 / libdsound / libxnet / libxact / libdmusic / …)
```

### Ship artifacts

| Output | Contents |
|--------|----------|
| `zig-out/lib/libc.lib` | picolibc + minimal libm + `libs/libc/xbox/*` runtime |
| `zig-out/lib/libcpp.lib` | LLVM libc++ + libcxxabi (freestanding profile) |
| `zig-out/lib/libkernel.lib` | Xbox kernel import library (from `libs/libkernel/xboxkrnl.def`) |
| `zig-out/lib/libxbdm.lib` | Xbox debug monitor import library |
| `zig-out/lib/libxapi.lib` | xAPI (k32 + dll + rtl + uuid + USB) |
| `zig-out/lib/libd3d8.lib` / `libd3d8i.lib` | Xbox D3D8 (NV2A) graphics driver (plain / D3DPERF-instrumented) |
| `zig-out/lib/libd3dx8.lib` | D3DX8 helper / utility library |
| `zig-out/lib/libxgraphics.lib` | XGraphics swizzle / texture utilities + XFONT |
| `zig-out/lib/libdsound.lib` | DirectSound (MCPX APU) audio |
| `zig-out/lib/libxmv.lib` | XMV video decoder |
| `zig-out/lib/libxnet.lib` / `libxneto.lib` | Xbox net stack — sockets / +ONLINE (QoS, SG) |
| `zig-out/lib/libxact.lib` | XACT audio engine |
| `zig-out/lib/libxonline.lib` / `libuix.lib` | Xbox Live client / UIX drop-in UI |
| `zig-out/lib/libxvoice.lib` | Xbox Communicator voice |
| `zig-out/lib/libdmusic.lib` | DirectMusic |
| `zig-out/include/` | picolibc + `xbox/` + `xboxkrnl/` + `c++/v1/` + public subsystem headers (`xt.h`/`xapi.h`/`xbox.h`/`xkbd.h`/`d3d8.h`/`dsound.h`/`xnet.h`/…) |

`build.ps1` packages the same set — plus `libcompat.lib` — into `dist\lib\{debug,release}` with the public headers in `dist\include`, ready to ship.

Library layering is one-way — `libxapi → libc → libkernel` and `libcpp → libc`, with the subsystem libs (`libd3d8`, `libdsound`, `libxnet`, …) layering above `libxapi` — so a C-only title can link `libc.lib` + `libkernel.lib` without dragging in xAPI or libc++.

The malloc family is the one exception: `libs/libc/xbox/heapalloc.c` allocates from the Xbox process heap (`RtlAllocateHeap` on `XapiProcessHeap`), so it reaches up into libxapi, where RXDK implements the RTL heap. Going through that heap is what gives every allocation the 16-byte alignment the XDK's SSE math code assumes, and it keeps `malloc`, `HeapAlloc`, `LocalAlloc` and `operator new` on one heap the way the retail XDK's CRT did. A program that calls `malloc` therefore also needs `libxapi.lib`, even if it uses nothing else from xAPI.

COFF archives from `zig lib` do not always resolve cleanly under `lld-link` with `--whole-archive`, so the dist also ships `libcompat.lib` — picolibc's own memcpy/math/CRT shims force-linked as loose objects — which an external title link should include alongside the staged `.lib`s.

### Target

- Triple: `x86-windows-gnu`
- C23 / C++23 (`-std=c23` / `-std=c++23`)
- Entry: `_start` from `libs/libc/xbox/crt0.S` (link with `-e start`)
- Debug output: `write` → `DbgPrint` (direct kernel import via `shared/include/xboxkrnl/`)

## Samples

This repo builds the libraries only. Sample titles and the XDK sample ports live in the separate [RXDK-Samples](https://github.com/Team-Resurgent/RXDK-Samples) repo, which links the libraries built here. For kit validation and XBE/ISO packaging notes, see [docs/kit-runbook.md](docs/kit-runbook.md).

## Design notes

- **Root-cause policy:** fix runtime/HAL bugs here rather than working around them in downstream titles.
- **C++ exceptions / EH:** DWARF/Itanium exceptions via vendored libunwind + libc++abi (`__cxa_throw`, `__gxx_personality_v0`, `.eh_frame`). Two Xbox-specific fixes: `main` runs on a dedicated `PsCreateSystemThreadEx` thread (the kernel's init-thread stack is too small for the unwinder), and `_LIBCXXABI_DTOR_FUNC` is forced to `__thiscall` (clang emits i386 member functions thiscall, but `-U_WIN32` would drop it). RTTI enabled for `dynamic_cast` / `typeinfo`.

See [docs/porting-notes.md](docs/porting-notes.md) for architecture and vendor mapping.
