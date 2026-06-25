# libxapi headers (legacy reference)

> **Superseded by [`libs/libxapi/`](../../libs/libxapi/).** The build and samples use `libs/libxapi/include/xapi.h` and the unified `libs/libxapi/xapi/` tree. This directory is kept for side-by-side diff only.

| Header | Role |
|--------|------|
| [`xapi.h`](xapi.h) | **Umbrella** — `xboxkrnl/xboxkrnl.h` + [`xdk_bridge.h`](xdk_bridge.h) (no `sdk/nt.h`) |
| [`xapip.h`](xapip.h) | Internal libxapi precompile (replaces leak `xapip.h`) |
| [`xapi_xtl.h`](xapi_xtl.h) | Minimal title surface (replaces `xtl.h`; **transitional** pull from `include/xdk/`) |
| [`xdk_bridge.h`](xdk_bridge.h) | Win32 typedefs when `winnt.h` is skipped |
| [`ntrtl.h`](ntrtl.h) | Rtl* shadow → `xboxkrnl/api/rtl.h` |
| [`nt_bridge.h`](nt_bridge.h) | Minimal ntdef for vendor `ntos.inc` (USB) |
| [`ntos.h`](ntos.h) | Shadow leak `ntos.h` for USB/C++ |

## Migration

`include/xdk/` is **transitional**. Declarations from `windef.h`, `winbase.h`, and `xbox.h` will move here (or into generated stubs) until `include/xdk` can be deleted. New work should not add dependencies on `include/sdk/`.

Build force-includes [`build/generated/xapi_site.h`](../build/generated/xapi_site.h) + [`picolibc.h`](../build/generated/picolibc.h). k32 TUs pull `xapi.h` through `xapip.h`.

Titles that need classic XDK APIs link **`libxapi.lib`**. Title samples use force-include [`build/generated/xapi_title_site.h`](../../build/generated/xapi_title_site.h) (`xboxkrnl` first, `NT_INCLUDED`, then `include/xtl.h`).

| Build step | Output |
|------------|--------|
| `zig build xapi-link` | `libc` + `libxapi.lib` link smoke (`GetTickCount`, `MulDiv`) |
| `zig build xapi-smoke` | 27 kit category tests (compile OK; full link in progress) |
| `zig build libxapi-core` | k32+dll+rtl+uuid subset (no USB OHCD) |

