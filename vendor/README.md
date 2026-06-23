# Vendor dependencies

Third-party sources only — no legacy CRT/STL/XDK copies.

## Submodules

| Path | Upstream | Used for |
|------|----------|----------|
| `vendor/picolibc/` | https://github.com/picolibc/picolibc | C library (`libxboxc.lib`) |
| `vendor/llvm-project/` | https://github.com/llvm/llvm-project | `libcxx/`, `libcxxabi/` (`libxboxcxx.lib`) |
| `vendor/xbox_leak_may_2020/` | https://github.com/xerohour/xbox_leak_may_2020 | Leak reference for `tools/generate_xboxkrnl_headers.py` (do not `#include` in app code) |

Shipped kernel declarations live in `include/xboxkrnl/` (generated). See `vendor/xbox_leak_may_2020/README.rxdk.md` and `tools/xboxkrnl_manifest/README.md`.

### Checkout

```powershell
git submodule update --init vendor/picolibc
git submodule update --init vendor/llvm-project
git submodule update --init vendor/xbox_leak_may_2020
```

`llvm-project` should use a **sparse checkout** limited to `libcxx` and `libcxxabi` (see `.git/modules/vendor/llvm-project/info/sparse-checkout` if configured).

## Build integration

- Wired from `build/picolibc.zig` and `build/libcxx.zig`
- Patches belong in `build/generated/` or `src/xbox/` — avoid editing vendor trees when possible
- picolibc config header: `build/generated/picolibc.h`
- libc++ site config: `build/generated/libcxx/__config_site`

## Not vendored

- Xbox kernel import library: `prebuilt/xboxkrnl.lib` (repo root)
- MSBuild / Windows SDK / MSVC runtime — forbidden in this repo's build graph
