# RXDK-LibsZig

Zig-only Xbox C/C++ runtime + clean-room xAPI for original Xbox devkits — **picolibc** + **LLVM libc++** + **libxapi**, ISO C23 / C++23.

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

## Layout

```
build.zig / build/         Zig build graph (orchestration + generated headers)
shared/include/            Public distributed headers (xt.h umbrella, xapi.h, xbox.h,
                           xkbd.h, windef/winbase, xboxkrnl/, xbox/)
shared/picolibc/           picolibc C headers (headers-only distribution)
shared/libcxx/             LLVM libc++ headers (headers-only distribution)
libs/libc/                 First-party libc runtime — xbox/ (HAL, crt0, kernel glue) + c23/ gap-fill (→ libc.lib)
libs/libcpp/               libc++ build orchestration over vendored libcxx (→ libcpp.lib)
libs/libxapi/              Clean-room xAPI port (→ libxapi.lib)
vendor/picolibc/           picolibc C library sources (submodule)
vendor/llvm-project/       libc++ / libcxxabi sources (submodule, sparse)
vendor/xbox_leak_may_2020/ XDK SDK headers (uuid) + kernel export reference (submodule)
prebuilt/xboxkrnl.lib      Only committed prebuilt (kernel import lib)
zig-out/lib/               libc.lib, libcpp.lib, libxapi.lib, libxapi_core.lib
zig-out/include/           Staged C / C++ / xAPI headers (after `zig build`)
samples/                   xapi-smoke + conformance PE smokes
build-iso.ps1              Interactive menu: pick a sample and build its ISO
```

## Build

```powershell
cd D:\Git\RXDK-LibsZig
.\build-iso.ps1                                # interactive menu → sample ISO
.\scripts\compile.ps1                          # libs + all samples (default)
.\scripts\compile.ps1 -Target libs            # libc/libcpp/libxapi + headers only
.\scripts\compile.ps1 -Target xapi-smoke      # single sample
.\scripts\compile.ps1 -Target xapi-smoke -Iso # PE → XBE → XISO (default.xbe at root)
```

Or invoke `zig build` directly:

```powershell
zig build verify-no-vs    # assert build/*.zig never invokes MSVC toolchain
zig build                 # libs + staged headers
zig build libxapi         # libxapi.lib only
zig build xapi-smoke      # 27-test xAPI smoke PE
zig build libc-smoke      # libc / C23 runtime matrix
zig build libcpp-smoke    # libc++ / C++23 smoke
```

### Ship artifacts

| Output | Contents |
|--------|----------|
| `zig-out/lib/libc.lib` | picolibc + minimal libm + `libs/libc/xbox/*` runtime |
| `zig-out/lib/libcpp.lib` | LLVM libc++ + libcxxabi (freestanding profile) |
| `zig-out/lib/libxapi.lib` | Clean-room xAPI (k32 + dll + rtl + uuid + USB) |
| `zig-out/lib/libxapi_core.lib` | xAPI without the USB stack (k32 + dll + rtl + uuid) |
| `zig-out/include/` | picolibc + `xbox/` + `xboxkrnl/` + `c++/v1/` + `xt.h`/`xapi.h`/`xbox.h`/`xkbd.h` |

Library layering is strictly one-way — `libxapi → libc → xboxkrnl` and `libcpp → libc` — so a C-only title can link `libc.lib` + `xboxkrnl.lib` without dragging in xAPI or libc++.

Samples link via direct object response files (`zig-out/link/*.rsp`) because COFF archives from `zig lib` do not always resolve cleanly under `lld-link` with `--whole-archive`. External consumers can link the staged `.lib`s or mirror the object-rsp pattern.

### Target

- Triple: `x86-windows-gnu`
- C23 / C++23 (`-std=c23` / `-std=c++23`)
- Entry: `_start` from `libs/libc/xbox/crt0.S` (link with `-e start`)
- Debug output: `write` → `DbgPrint` (direct kernel import via `shared/include/xboxkrnl/`)

## Samples

| Step | PE | Notes |
|------|-----|-------|
| `xapi-smoke` | `zig-out/samples/xapi-smoke/xapi-smoke.exe` | 27 xAPI category tests (kit hardware + HDD) |
| `libc-smoke` | `zig-out/samples/libc-smoke/libc-smoke.exe` | libc / C23 runtime matrix incl. `<stdbit.h>` (see `docs/conformance.md`) |
| `libcpp-smoke` | `zig-out/samples/libcpp-smoke/libcpp-smoke.exe` | libc++ / C++23 — 51 tests incl. exceptions, coroutines, `<format>`/`<print>`, `<fstream>`, `<ranges>`, `<chrono>`, `<thread>` + atomic-wait/`<latch>`/`<semaphore>`/`<barrier>`, `<pmr>`, `<valarray>`, `<charconv>`, `<regex>`, `<filesystem>` (see `docs/conformance.md`) |

Kit validation and XBE/ISO packaging: see [docs/kit-runbook.md](docs/kit-runbook.md), or just run [`build-iso.ps1`](build-iso.ps1).

## Design notes

- **Reference only:** [RXDK-Libs](https://github.com/Team-Resurgent/RXDK-Libs) for behavior (startup, printf path, kernel exports). Nothing is copied verbatim from legacy CRT/STL trees.
- **Root-cause policy:** fix runtime/HAL here; do not patch samples to dodge library bugs.
- **C++ exceptions / EH:** DWARF/Itanium exceptions via vendored libunwind + libc++abi (`__cxa_throw`, `__gxx_personality_v0`, `.eh_frame`). Two Xbox-specific fixes: `main` runs on a dedicated `PsCreateSystemThreadEx` thread (the kernel's init-thread stack is too small for the unwinder), and `_LIBCXXABI_DTOR_FUNC` is forced to `__thiscall` (clang emits i386 member functions thiscall, but `-U_WIN32` would drop it). RTTI enabled for `dynamic_cast` / `typeinfo`.

See [docs/porting-notes.md](docs/porting-notes.md) for architecture and vendor mapping.
