# Porting notes — RXDK-Libs greenfield runtime

RXDK-LibsOld is a **reference** for behavior (startup order, printf path, kernel exports). Do not copy MSVC CRT, XDK headers, or shim layers.

## Architecture

```
picolibc (C) + minimal libm
        +
libs/libc/xbox/* (HAL, crt0, kernel, stubs)
        = libc.lib

LLVM libc++ + libcxxabi (freestanding, picolibc locale backend)
        = libcpp.lib
```

| Layer | Location |
|-------|----------|
| First-party Xbox glue | `libs/libc/xbox/`, `libs/libc/c23/` |
| Third-party C | `vendor/picolibc/` |
| Third-party C++ | `vendor/llvm-project/libcxx`, `libcxxabi` |
| Public headers | staged to `zig-out/include/` |
| Kernel import lib | `prebuilt/xboxkrnl.lib` (only prebuilt) |
| Kernel headers | `shared/include/xboxkrnl/` — umbrella `xboxkrnl.h`, base `xboxdef.h`, `types/*.h`, `api/*.h`, optional `winnt/{pe,xbe}.h` |

## Symbol / ABI notes (x86-windows-gnu)

- C functions must **not** use a leading underscore in source when targeting the PE ABI layer (e.g. implement `write`, not `_write`; `write` links as `_write`).
- Asm `crt0.S` defines `_start`; link with `-e start` (not `-e _start`).
- Zig/lld emits Windows subsystem 3 (CONSOLE); no PE pre-patch is needed — `imagebld` (RXDK-Tools) coerces the subsystem to **14** (`IMAGE_SUBSYSTEM_XBOX`) and resolves the real TLS directory itself when building the XBE.
- `.bss` is merged into `.data` by lld (`VirtualSize > SizeOfRawData`). `imagebld` (RXDK-Tools, build-78+) zero-fills the emitted `.data` so `SizeOfRawData == VirtualSize` in the XBE and the loader copies the zeroed tail — uninitialized globals boot as zero with no runtime fixup. (Historically a per-title `image_init.c` two-pass zeroed this range at startup; removed once the imagebld fix was HW-verified.)
- lld on PE does not honour picolibc `__weak_reference` for `stdin`/`stdout`/`stderr`. Vendor `posixiob_*.c` is excluded; `libs/libc/xbox/posix_stdio_streams.c` defines strong `FILE *const stdin/stdout/stderr` alongside `__posix_*`.
- `___main` comes from C `__main` in `libs/libc/xbox/startup.c`; it walks MinGW/lld `__CTOR_LIST__` so libc++ global ctors (including `std::cout` init) run before C++ `main` body executes.
- `prebuilt/xboxkrnl.lib` with **no** `--whole-archive` (duplicate import descriptors break the kit loader). `DbgPrint` is import ordinal **8** per OpenXDK `xboxkrnl.exe.def`.

## libc++ freestanding profile

Generated site config: `build/generated/libcxx/__config_site`

- Single-threaded: `_LIBCPP_HAS_NO_THREADS`, `_LIBCXXABI_HAS_NO_THREADS`
- picolibc locale backend: `_LIBCPP_LIBC_NEWLIB`, `-U_WIN32`, `-U__MINGW32__` when compiling libc++
- Prerequisite header: `build/generated/libcxx/picolibc_prereq.h` (`_POSIX_C_SOURCE`, `_GNU_SOURCE` for `strtod_l`, etc.)
- Exceptions disabled for now (`_LIBCPP_HAS_NO_EXCEPTIONS`, `cxa_noexception.cpp`); libunwind bring-up tracked separately

## picolibc profile

Generated config: `build/generated/picolibc.h`

- Tiny stdio, single-thread, global errno
- POSIX locale API enabled (`__HAVE_POSIX_LOCALE_API`)
- Excludes: x86 asm fragments, TLS/interrupt, duplicate `posix_locale.c`, heavy libm paths (selective `libm/common` + `libm/ld` instead)

## Linking samples

Internal smokes link picolibc + xbox + (optional) libcxx **objects** via `@zig-out/link/<sample>.rsp` plus `prebuilt/xboxkrnl.lib` and `prebuilt/xboxkrnl_xbld.obj` (`.XBLD` / kernel lib version). Do **not** use `--whole-archive` on `xboxkrnl.lib` — it duplicates import descriptors and breaks kit load.

External titles just link the shipped `.lib` files — the two title-link objects are baked into the archives so no loose objects are needed:
- `xboxkrnl_xbld.obj` (`.XBLD` / `_XboxKrnlBuildNumber`) is packed into **libc.lib**; `libs/libc/xbox/startup.c` holds a genuine reference to `XboxKrnlBuildNumber` so the always-linked startup pulls the member (a `#pragma comment(linker,"/include:")` directive does **not** work for archive pull on the x86-windows-gnu toolchain).
- `xapi_start.obj` (`XapiTitleStartup`, title-compiled) is packed into **libxapi.lib** (and `libxapi_core.lib`); any title that links libxapi resolves the entry from the archive via `-e XapiTitleStartup`.

## Intentional divergences from RXDK-LibsOld

| RXDK-LibsOld | RXDK-Libs |
|-----------|--------------|
| `libcmt.lib` / MSVC STL | `libc.lib` / `libcpp.lib` |
| `mainCRTStartup`, XAPILIB | `_start` + direct kernel import |
| MSBuild / `cl.exe` | `zig build` only |
| Full libm / EH / threads | Minimal libm; no EH yet; single-thread |

## Kit validation

See [kit-runbook.md](kit-runbook.md).
