# libxapi (Phase 1)

Self-contained Xbox xAPI package. All library sources and internal headers live under `xapi/`; titles include a single public header.

## Layout

| Path | Role |
|------|------|
| `include/xapi.h` | **Only** installed public header (`zig-out/include/xapi.h`) |
| `xapi/site/` | Frozen profile + slice bridges (`profile.h`, `bridge_*.h`) |
| `xapi/internal/` | Compile headers (`compile.h`, `precompile.h`, bridges) |
| `xapi/win32/`, `xapi/nt/` | Trimmed Win32 / NT declarations for vendor sources |
| `xapi/k32/`, `dll/`, `rtl/`, `uuid/`, `usb/` | Vendor sources (by domain) |
| `xapi/port/` | RXDK gap-fix sources |
| `xapi/minilib/` | Optional string/mem helpers (not linked when titles use `libc.lib`) |
| `sources.zig` | Frozen compile manifest |
| `build.zig` | Package build (imported from repo root `build.zig`) |

## Build

```powershell
zig build libxapi
zig build xapi-slices
zig build xapi-smoke
zig build xapi-link
```

External dependencies (repo root only): `include/xboxkrnl/`, `vendor/picolibc/`, `prebuilt/xboxkrnl.lib`. Titles also link `libc.lib` for CRT/string routines.

## Legacy reference

Repo-root paths (`include/xapi/`, `build/xapi.zig`, `vendor/xbox_private/private/`) are **frozen reference copies** — not used by the build after Phase 1. Do not delete them when diffing against this package.
