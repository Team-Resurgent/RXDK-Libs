# Porting notes — RXDK-LibsZig greenfield runtime

RXDK-Libs is a **reference** for behavior (startup order, printf path, kernel exports). Do not copy MSVC CRT, XDK headers, or shim layers.

## Architecture

```
picolibc (C) + minimal libm
        +
src/xbox/* (HAL, crt0, kernel, stubs)
        = libxboxc.lib

LLVM libc++ + libcxxabi (freestanding, picolibc locale backend)
        = libxboxcxx.lib
```

| Layer | Location |
|-------|----------|
| First-party Xbox glue | `src/xbox/`, `src/runtime/c23/` |
| Third-party C | `vendor/picolibc/` |
| Third-party C++ | `vendor/llvm-project/libcxx`, `libcxxabi` |
| Public headers | staged to `zig-out/include/` |
| Kernel import lib | `prebuilt/xboxkrnl.lib` (only prebuilt) |
| Kernel headers | `include/xboxkrnl/` — umbrella `xboxkrnl.h`, base `xboxdef.h`, `types/*.h`, `api/*.h`, optional `winnt/{pe,xbe}.h` |

## Symbol / ABI notes (x86-windows-gnu)

- C functions must **not** use a leading underscore in source when targeting the PE ABI layer (e.g. implement `write`, not `_write`; `write` links as `_write`).
- Asm `crt0.S` defines `_start`; link with `-e start` (not `-e _start`).
- After link, `scripts/Patch-PeXbox.ps1` sets PE **subsystem 14** (`IMAGE_SUBSYSTEM_XBOX`) — required for valid `imagebld` output (Zig/lld only emits Windows subsystem 3 today).
- `imagebld` only embeds part of `.data` in the XBE (see `SizeOfRawData` in `imagebld /DUMP`, often less than the PE `SizeOfRawData`). `Write-XboxImageInit.ps1` probe-links, converts to XBE, and emits `zig-out/link/<sample>_image_init.h` with the **XBE** tail size; `image_init.c` clears that range at startup before `printf`.
- lld on PE does not honour picolibc `__weak_reference` for `stdin`/`stdout`/`stderr`. Vendor `posixiob_*.c` is excluded; `src/xbox/posix_stdio_streams.c` defines strong `FILE *const stdin/stdout/stderr` alongside `__posix_*`.
- `___main` comes from C `__main` in `src/xbox/startup.c`; it walks MinGW/lld `__CTOR_LIST__` so libc++ global ctors (including `std::cout` init) run before C++ `main` body executes.
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

External titles can use the shipped `.lib` files; if `lld-link` drops archive members, use the same object-rsp approach or `--whole-archive` on both libs.

## Intentional divergences from RXDK-Libs

| RXDK-Libs | RXDK-LibsZig |
|-----------|--------------|
| `libcmt.lib` / MSVC STL | `libxboxc.lib` / `libxboxcxx.lib` |
| `mainCRTStartup`, XAPILIB | `_start` + direct kernel import |
| MSBuild / `cl.exe` | `zig build` only |
| Full libm / EH / threads | Minimal libm; no EH yet; single-thread |

## Kit validation

See [kit-runbook.md](kit-runbook.md).
