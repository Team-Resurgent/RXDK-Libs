# RXDK-LibsZig

Zig-only Xbox C/C++ runtime for original Xbox devkits — **picolibc** + **LLVM libc++**, ISO C23 / C++23.

No Visual Studio, MSBuild, `cl.exe`, or Windows SDK is required to build the runtime in this repo. Host deploy tools (`imagebld`, neighborhood, etc.) live in [RXDK-Libs](https://github.com/Team-Resurgent/RXDK-Libs) and may stay MSBuild-based.

## Prerequisites

- [Zig](https://ziglang.org/) **0.16+** (tested with 0.16.0)
- Git submodules: `vendor/picolibc`, `vendor/llvm-project` (sparse checkout for `libcxx` + `libcxxabi`), `vendor/xbox_leak_may_2020` (export ordinals reference only)

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
build.zig / build/       Zig build graph (only orchestration)
src/xbox/                Xbox HAL, crt0, kernel glue (first-party)
src/runtime/c23/         Small C23 gap-fill (e.g. stdbit)
vendor/picolibc/         C library sources (submodule)
vendor/llvm-project/     libc++ / libcxxabi sources (submodule, sparse)
prebuilt/xboxkrnl.lib    Only committed prebuilt (kernel import lib)
include/xboxkrnl/         Shipped kernel headers (xboxkrnl.h umbrella + api/*.h)
vendor/xbox_leak_may_2020/ Reference submodule for header generation only
zig-out/lib/             libxboxc.lib, libxboxcxx.lib
zig-out/include/         Staged C/C++ headers (after `zig build`)
samples/                 PE smokes linked with zig cc + lld
```

## Build

```powershell
cd D:\Git\RXDK-LibsZig
.\scripts\compile.ps1                  # libs + all samples (default)
.\scripts\compile.ps1 -Target libs     # libxboxc.lib, libxboxcxx.lib, headers only
.\scripts\compile.ps1 -Target hello-c  # single sample
.\scripts\compile.ps1 -Xbe             # also PE → XBE via RXDK-Tools imagebld
.\scripts\compile.ps1 -Iso             # PE → XBE → XISO (default.xbe at root)
```

Or invoke `zig build` directly:

```powershell
zig build verify-no-vs    # assert build/*.zig never invokes MSVC toolchain
zig build                 # libs + headers + kernel-smoke + hello-c
zig build hello-cpp       # iostream sample
zig build conformance-c
zig build conformance-c23
zig build conformance-cpp23
```

### Ship artifacts

| Output | Contents |
|--------|----------|
| `zig-out/lib/libxboxc.lib` | picolibc + minimal libm + `src/xbox/*` |
| `zig-out/lib/libxboxcxx.lib` | LLVM libc++ + libcxxabi (freestanding profile) |
| `zig-out/include/` | picolibc + `include/xbox/` + `include/xboxkrnl/` + `c++/v1/` |

Samples link via direct object response files (`zig-out/link/*.rsp`) because COFF archives from `zig lib` do not always resolve cleanly under `lld-link` with `--whole-archive`. External consumers can link `libxboxc.lib` / `libxboxcxx.lib` or mirror the object-rsp pattern.

### Target

- Triple: `x86-windows-gnu`
- C23 / C++23 (`-std=c23` / `-std=c++23`)
- Entry: `_start` from `src/xbox/crt0.S` (link with `-e start`)
- Debug output: `write` → `DbgPrint` (direct kernel import via `include/xboxkrnl/`)

## Samples

| Step | PE | Notes |
|------|-----|-------|
| `kernel-smoke` | `zig-out/samples/kernel-smoke/kernel-smoke.exe` | `DbgPrint` only; `xboxkrnl.lib` |
| `hello-c` | `zig-out/samples/hello-c/hello-c.exe` | `printf` via picolibc |
| `hello-cpp` | `zig-out/samples/hello-cpp/hello-cpp.exe` | `std::cout` via libc++ |
| `conformance-c` | `conformance-c.exe` | libc/C23 runtime matrix (16 tests, see `docs/conformance.md`) |
| `conformance-c23` | `c23-stdbit-smoke.exe` | `<stdbit.h>` smoke |
| `conformance-cpp23` | `cpp23-expected-smoke.exe` | `<expected>`, `<string_view>` |

Kit validation and XBE packaging: see [docs/kit-runbook.md](docs/kit-runbook.md).

## Design notes

- **Reference only:** [RXDK-Libs](https://github.com/Team-Resurgent/RXDK-Libs) for behavior (startup, printf path, kernel exports). Nothing is copied verbatim from legacy CRT/STL trees.
- **Root-cause policy:** fix runtime/HAL here; do not patch samples to dodge library bugs.
- **C++ exceptions / EH:** current libc++ profile is `-fno-exceptions` with `cxa_noexception.cpp` until libunwind is wired for Xbox PE. RTTI remains enabled for `dynamic_cast` / `typeinfo`.

See [docs/porting-notes.md](docs/porting-notes.md) for architecture and vendor mapping.
