# RXDK-Libs

Zig-built Xbox C/C++ runtime and SDK for original Xbox devkits — **picolibc** + **LLVM libc++**, a clean-room **xAPI**, and the **D3D8 / D3DX8 / DirectSound / XGraphics / XMV / XNet** subsystem libraries, ISO C23 / C++23.

No Visual Studio, MSBuild, `cl.exe`, or Windows SDK is required to build the runtime in this repo. Host deploy tools (`imagebld`, `xdvdfs`, neighborhood, etc.) live under `tools/` and may stay MSBuild-based.

## Prerequisites

- [Zig](https://ziglang.org/) **0.16+** (tested with 0.16.0)
- Git submodules: `vendor/picolibc`, `vendor/llvm-project` (sparse checkout for `libcxx` + `libcxxabi`), `vendor/xbox_leak_may_2020` (XDK SDK headers for the uuid slice + kernel export ordinals)

```powershell
.\scripts\init-submodules.ps1
```

Or manually:

```powershell
git submodule update --init vendor/picolibc vendor/xbox_leak_may_2020 vendor/llvm-project
git -C vendor/llvm-project sparse-checkout init --cone
git -C vendor/llvm-project sparse-checkout set libcxx libcxxabi
```

The xAPI and subsystem libraries (`libxapi`, `libd3d8`, `libd3dx8`, `libdsound`, `libxgraphics`, `libxmv`, `libxnet`) build over the Xbox source tree in `vendor/xbox_private/`, which is **not** committed here. Populate it from a local [RXDK-LibsOld](https://github.com/Team-Resurgent/RXDK-LibsOld) checkout:

```powershell
# RXDK-LibsOld cloned beside this repo, or set $env:RXDK_LIBS_ROOT to its path
.\scripts\sync-xapi-vendor.ps1
```

`libc`, `libcpp`, and `libkernel` build without it.

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
libs/libxapi/              Clean-room xAPI port (→ libxapi.lib / libxapi_core.lib)
libs/libd3d8/              Xbox D3D8 (NV2A) graphics driver (→ libd3d8.lib)
libs/libd3dx8/             D3DX8 helper / utility library (→ libd3dx8.lib)
libs/libdsound/            DirectSound (MCPX APU) audio (→ libdsound.lib)
libs/libxgraphics/         XGraphics swizzle / texture utilities (→ libxgraphics.lib)
libs/libxmv/               XMV video decoder (→ libxmv.lib)
libs/libxnet/              Xbox net stack — XNet / winsock (→ libxnet.lib)
vendor/picolibc/           picolibc C library sources (submodule)
vendor/llvm-project/       libc++ / libcxxabi sources (submodule, sparse)
vendor/xbox_leak_may_2020/ XDK SDK headers (uuid) + kernel export reference (submodule)
vendor/xbox_private/       Xbox subsystem sources (local-only, synced from RXDK-LibsOld)
prebuilt/                  Committed kernel object shims (DbgPrint / descriptor / .XBLD) used at link time
zig-out/lib/               Staged .lib outputs + object response files
zig-out/include/           Staged C / C++ / xAPI / subsystem headers (after `zig build`)
samples/                   Conformance smokes + D3D8 / DirectSound / XMV / XNet / input demos
build-iso.ps1              Interactive menu: pick a sample and build its ISO
```

## Build

```powershell
cd D:\Git\RXDK-Libs
.\build-iso.ps1                                # interactive menu → sample ISO
.\scripts\compile.ps1                          # libs + all samples (default)
.\scripts\compile.ps1 -Target libs            # libraries + headers only
.\scripts\compile.ps1 -Target xapi-smoke      # single sample
.\scripts\compile.ps1 -Target xapi-smoke -Iso # PE → XBE → XISO (default.xbe at root)
```

Or invoke `zig build` directly:

```powershell
zig build verify-no-vs    # assert build/*.zig never invokes MSVC toolchain
zig build                 # libs + staged headers
zig build libkernel       # libkernel.lib (kernel import lib) only
zig build libxapi         # libxapi.lib only
zig build libd3d8         # a single subsystem lib (libd3d8 / libdsound / libxnet / …)
zig build libc-smoke      # libc / C23 runtime matrix
zig build libcpp-smoke    # libc++ / C++23 smoke
zig build xapi-smoke      # 27-test xAPI smoke PE
```

### Ship artifacts

| Output | Contents |
|--------|----------|
| `zig-out/lib/libc.lib` | picolibc + minimal libm + `libs/libc/xbox/*` runtime |
| `zig-out/lib/libcpp.lib` | LLVM libc++ + libcxxabi (freestanding profile) |
| `zig-out/lib/libkernel.lib` | Xbox kernel import library (from `libs/libkernel/xboxkrnl.def`) |
| `zig-out/lib/libxapi.lib` | Clean-room xAPI (k32 + dll + rtl + uuid + USB) |
| `zig-out/lib/libxapi_core.lib` | xAPI without the USB stack (k32 + dll + rtl + uuid) |
| `zig-out/lib/libd3d8.lib` | Xbox D3D8 (NV2A) graphics driver |
| `zig-out/lib/libd3dx8.lib` | D3DX8 helper / utility library |
| `zig-out/lib/libdsound.lib` | DirectSound (MCPX APU) audio |
| `zig-out/lib/libxgraphics.lib` | XGraphics swizzle / texture utilities |
| `zig-out/lib/libxmv.lib` | XMV video decoder |
| `zig-out/lib/libxnet.lib` | Xbox net stack (XNet / winsock) |
| `zig-out/include/` | picolibc + `xbox/` + `xboxkrnl/` + `c++/v1/` + public subsystem headers (`xt.h`/`xapi.h`/`xbox.h`/`xkbd.h`/`d3d8.h`/`dsound.h`/`xnet.h`/…) |

Library layering is strictly one-way — `libxapi → libc → libkernel` and `libcpp → libc`, with the subsystem libs (`libd3d8`, `libdsound`, `libxnet`, …) layering above `libxapi` — so a C-only title can link `libc.lib` + `libkernel.lib` without dragging in xAPI or libc++.

Samples link via direct object response files (`zig-out/link/*.rsp`) because COFF archives from `zig lib` do not always resolve cleanly under `lld-link` with `--whole-archive`. External consumers can link the staged `.lib`s or mirror the object-rsp pattern.

### Target

- Triple: `x86-windows-gnu`
- C23 / C++23 (`-std=c23` / `-std=c++23`)
- Entry: `_start` from `libs/libc/xbox/crt0.S` (link with `-e start`)
- Debug output: `write` → `DbgPrint` (direct kernel import via `shared/include/xboxkrnl/`)

## Samples

| Sample | PE | Notes |
|--------|-----|-------|
| `xapi-smoke` | `zig-out/samples/xapi-smoke/xapi-smoke.exe` | 27 xAPI category tests (kit hardware + HDD) |
| `xapi-input` | `zig-out/samples/xapi-input/xapi-input.exe` | Controller / keyboard input via xAPI |
| `libc-smoke` | `zig-out/samples/libc-smoke/libc-smoke.exe` | libc / C23 runtime matrix incl. `<stdbit.h>` (see `docs/conformance.md`) |
| `libcpp-smoke` | `zig-out/samples/libcpp-smoke/libcpp-smoke.exe` | libc++ / C++23 — 59 tests incl. exceptions, coroutines + `std::generator`, `thread_local`, `<format>`/`<print>`, `<fstream>`, `<ranges>`, `<chrono>`, `<thread>` + atomic-wait/`<latch>`/`<semaphore>`/`<barrier>`, `<pmr>`, `<valarray>`, `<charconv>`, `<regex>`, `<filesystem>`, `<any>`/`<mdspan>`/`<flat_map>`/`std::bind` (see `docs/conformance.md`) |
| `d3d8-triangle` | `zig-out/samples/d3d8-triangle/d3d8-triangle.exe` | D3D8 (NV2A) gouraud triangle |
| `d3d8-textures` | `zig-out/samples/d3d8-textures/d3d8-textures.exe` | D3D8 textured rendering |
| `dsound-music` | `zig-out/samples/dsound-music/dsound-music.exe` | DirectSound audio playback |
| `xmv-play` | `zig-out/samples/xmv-play/xmv-play.exe` | XMV video playback |
| `xnet-net` | `zig-out/samples/xnet-net/xnet-net.exe` | XNet networking |

Kit validation and XBE/ISO packaging: see [docs/kit-runbook.md](docs/kit-runbook.md), or just run [`build-iso.ps1`](build-iso.ps1).

## Design notes

- **Reference only:** [RXDK-LibsOld](https://github.com/Team-Resurgent/RXDK-LibsOld) for behavior (startup, printf path, kernel exports). Nothing is copied verbatim from legacy CRT/STL trees.
- **Root-cause policy:** fix runtime/HAL here; do not patch samples to dodge library bugs.
- **C++ exceptions / EH:** DWARF/Itanium exceptions via vendored libunwind + libc++abi (`__cxa_throw`, `__gxx_personality_v0`, `.eh_frame`). Two Xbox-specific fixes: `main` runs on a dedicated `PsCreateSystemThreadEx` thread (the kernel's init-thread stack is too small for the unwinder), and `_LIBCXXABI_DTOR_FUNC` is forced to `__thiscall` (clang emits i386 member functions thiscall, but `-U_WIN32` would drop it). RTTI enabled for `dynamic_cast` / `typeinfo`.

See [docs/porting-notes.md](docs/porting-notes.md) for architecture and vendor mapping.
