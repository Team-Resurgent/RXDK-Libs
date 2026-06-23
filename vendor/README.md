# Vendor dependencies

Third-party sources only — no legacy CRT/STL/XDK copies.

## Submodules

| Path | Upstream | Used for |
|------|----------|----------|
| `vendor/picolibc/` | https://github.com/picolibc/picolibc | C library (`libxboxc.lib`) |
| `vendor/llvm-project/` | https://github.com/llvm/llvm-project | `libcxx/`, `libcxxabi/` (`libxboxcxx.lib`) |

### Checkout

```powershell
git submodule update --init vendor/picolibc
git submodule update --init vendor/llvm-project
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
