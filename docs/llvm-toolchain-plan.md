# RXDK-Libs on the Team-Resurgent LLVM toolchain (in place of zig)

Status: **scoping / experiment** on branch `llvm-toolchain`. Goal: build the RXDK libs with
the Team-Resurgent `xboxog` clang/lld/llvm-lib (published at
`Team-Resurgent/llvm-project` → release **"Team Resurgent clang latest"**,
`xboxog-<os>-<arch>.zip`) instead of the bundled `zig` compiler, so we own the exact
LLVM version + patches and drop the zig dependency.

## How the build uses zig today

`zig` is invoked from three places (all via `b.graph.zig_exe` in `build/`):

| Role | Where | Command today | Notes |
|---|---|---|---|
| **Compile** C/C++/asm → `.o` | `build/compile_c.zig` (`addBatch`) | `zig cc` / `zig c++` / `zig cc` (for `.s`) `-march=pentium3 -target x86-windows-gnu -c -o …` | `zig cc` **is** clang; RXDK passes `-nostdinc -ffreestanding -fno-builtin` and its own SDK headers + vendored **picolibc**, so it does **not** rely on zig's bundled MinGW headers/CRT. |
| **Archive** `.o` → `.lib` | `build/coff_lib.zig` (`pack`) | `zig lib /NOLOGO /OUT:… @rsp` | `zig lib` == **llvm-lib** (the MSVC COFF librarian). |
| **Link** title → `.xbe` | **NOT here** — done by the VS20XX toolset `ZigLd` (`zig cc`) in RXDK-Tools | — | This is where zig's **compiler-rt builtins** are pulled in. Out of scope for Phase 1. |

Target: `x86-windows-gnu` (zig) = **`i686-pc-windows-gnu`** (clang). CPU pinned `-march=pentium3`
(PIII: MMX+SSE1, no SSE2 — SSE2 faults on HW).

`libcompat.lib` (`build.ps1`) is assembled from RXDK's **own** compiled objects (picolibc
`fabs/sqrt/sin/…`, the MSVC `__alldiv/__aullrem/…` shim in `msvc_lldiv.c`, and the COMDAT-fix
objects) specifically to **override** zig's bundled compiler-rt (whose ABI-mismatched `fabs`
corrupts the x87 stack, and whose weak SSE2 `memmove` must lose to libc's). It is **not** sourced
from zig — so it carries over unchanged.

## What the xboxog release ships vs. what we need

Release zip `bin/`: `clang`, `clang++`, `ld.lld`, `lld`, `llvm-ar`, `llc` (+ `lib/clang` resource
dir, and `libcxx`/`libcxxabi`/`libunwind`/`libc` headers+src). Built with
`LLVM_ENABLE_PROJECTS="clang;lld"`, `LLVM_TARGETS_TO_BUILD=X86`.

| Need | In release? | Action |
|---|---|---|
| **clang** (compile) | ✅ | use `clang --target=i686-pc-windows-gnu` |
| **llvm-lib** (COFF librarian, replaces `zig lib`) | ❌ (only `llvm-ar` = GNU ar) | **add `llvm-lib` to the CI build target + Package step**, OR verify `llvm-ar` GNU `.a` is accepted by our lld title-link (lld reads GNU archives; but consumers expect COFF `.lib`). Prefer adding `llvm-lib`. |
| **ld.lld** (link) | ✅ | not used in Phase 1 (link stays zig) |
| **compiler-rt builtins** (title link) | ❌ (not in ENABLE_PROJECTS) | **not needed for Phase 1** — title link stays on zig `cc`, which supplies them. Needed only for Phase 2. |

## Phase 1 — swap compile + archive only (this branch)

Keep the VS20XX toolset link on zig. Only change RXDK-Libs' compiler + librarian.

1. **Toolchain staging**: add a way to fetch/stage `xboxog-windows-x64.zip` (like zig is staged
   today) and point the build at its `clang`/`llvm-lib`. Introduce a `RXDK_LLVM` (or reuse the
   staged-tools layout) env/param → the toolchain `bin/`.
2. **`build/compile_c.zig`**: replace `b.graph.zig_exe` + `"cc"/"c++"` with the staged `clang`
   (`--target=i686-pc-windows-gnu` in place of `-target x86-windows-gnu`). Keep every flag
   (`-march=pentium3`, `-nostdinc`, `-ffreestanding`, `-fno-builtin`, `-femulated-tls`, the `-Wno-…`
   set, opt flag, include dirs) verbatim.
3. **`build/coff_lib.zig`**: replace `zig lib /NOLOGO /OUT:… @rsp` with `llvm-lib /NOLOGO /OUT:… @rsp`
   (llvm-lib takes the same MSVC-style switches).
4. Leave `build.ps1` / libcompat / picolibc vendoring untouched.

### Verify
- Build one lib (e.g. `libc`, then `libxapi`) and **byte-compare** the `.o` and `.lib` against the
  zig output. ⚠️ Byte-identical requires the **same clang/LLVM version** zig bundles — confirm the
  xboxog LLVM version vs zig's bundled clang; if they differ, expect functionally-equivalent but not
  byte-identical objects (RXDK has valued byte-identical objs — decide if we re-baseline).
- Then build all libs (`build.ps1`) and run the sample sweep on xemu/HW.

## Phase 2 — remove zig entirely (later)

Move the **title link** off zig too (the VS20XX toolset `ZigLd`/`ZigCompile`, RXDK-Tools):
- Drive `clang --target=… -fuse-ld=lld` (or `ld.lld` directly) for the title link.
- Supply **compiler-rt builtins** for `i686-windows-gnu` — add `compiler-rt` (builtins) to the
  xboxog LLVM CI (`LLVM_ENABLE_RUNTIMES`/`PROJECTS`) cross-built for i386, packaged as
  `libclang_rt.builtins-i386.a`; or compile the needed builtins from the vendored
  `vendor/llvm-project/compiler-rt` source (we already carry it).
- Re-validate the libcompat override still wins over the new compiler-rt.

## Open questions / risks
- **llvm-lib packaging** — needs a one-line CI change on the `xboxog` branch's build workflow.
- **Version parity** — zig's clang vs xboxog LLVM; affects byte-identical reproducibility.
- **`-target x86-windows-gnu` vs `--target=i686-pc-windows-gnu`** — confirm clang defaults (data
  layout, `__GNUC__`, wchar) match what zig cc produced; RXDK's `-nostdinc`+own headers minimise
  the surface but the builtin predefines still matter.
- **`.s`/`.asm`** — `.asm` is skipped in `compile_c`; `.s` goes through `cc` (clang integrated asm) — carries over.
