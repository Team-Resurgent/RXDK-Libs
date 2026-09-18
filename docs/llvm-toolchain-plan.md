# RXDK-Libs on the Team-Resurgent LLVM toolchain (in place of zig)

Status: **scoping / experiment** on branch `llvm-toolchain`. Goal: **stop using zig entirely** and
drive everything with the Team-Resurgent LLVM toolchain, so we own the exact LLVM version + patches.

Like zig, this is **per host OS/CPU, cross-compiling to the Xbox target**: the developer/CI runs the
native host toolchain (`xboxog-<os>-<arch>.zip` — one of win-x64, win-arm64, linux-x64, linux-arm64,
macos-x64, macos-arm64, from the `Team-Resurgent/llvm-project` release **"Team Resurgent clang
latest"**), and clang cross-compiles to `i686-pc-windows-gnu` (the OG Xbox). The host-zip is picked
by the running OS/arch exactly as the host tools do today (cf. `RxdkPaths.ToolRid`
win-x64/linux-x64/…). Same model as zig (one host toolchain, cross to Xbox), just LLVM instead of zig.

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

### Verify — parity checks (do NOT assume; zig stung us on SSE2)

The whole migration must be gated on parity, and the codegen/instruction-set checks matter most —
a compiler that emits **SSE2+** faults as `STATUS_ILLEGAL_INSTRUCTION` on real HW (PIII = MMX+SSE1
only), and **xemu often masks it** ([[rxdk-trees-fur-xemu-gap]]). So parity is checked in this order:

1. **Instruction-set scan (the critical gate).** Disassemble every emitted `.o`/`.lib`
   (`llvm-objdump -d`) and fail the build on any instruction above the PIII/SSE1 baseline
   (any SSE2/SSE3/SSSE3/SSE4/AVX/… opcode). Build this as a scripted check that runs on both the
   zig output (baseline) and the LLVM output and diffs the flagged-opcode set — it must be **empty**
   for LLVM just as it is for zig. This catches the SSE2 class before HW.
2. **Byte / functional diff of objects.** Byte-compare `.o` + `.lib` vs zig. Byte-identical only
   holds if the xboxog LLVM version == zig's bundled clang version; otherwise expect
   functionally-equivalent output (symbol table, section layout, relocations) — verify those match
   and decide whether to re-baseline the "byte-identical" expectation.
3. **ABI / symbol parity.** Same name mangling, `__cdecl`/`__stdcall` decoration, struct layout and
   `wchar_t`/enum sizes, and predefined macros (`__GNUC__`, data layout) — the libs link against
   MSVC-ABI title code and the leak's objects, so any drift breaks linking or corrupts calls
   (cf. [[rxdk-notifier-fence-bug]], the `__cdecl`-under-stdcall miscompile).
4. **compiler-rt behaviour (Phase 2).** When the title link moves to LLVM, re-check the `build.ps1`
   hazards: no ABI-mismatched x87 `fabs`, no weak **SSE2** `memmove` sneaking in — `libcompat` must
   still win ([[rxdk-memmove-builtin-recursion-fix.md]]).
5. **Real-hardware sweep.** Because xemu masks the SSE2/illegal-instruction class, run the sample
   sweep on **real HW**, not just xemu — that is the final parity gate.

## Phase 2 — apps/titles on LLVM too (the planned end state)

**Confirmed direction:** once the libs are built with LLVM, the **apps/titles that consume them
also build with LLVM** — i.e. the whole toolchain drops zig, not just RXDK-Libs. This is the
title **compile + link** path, which lives in the **RXDK-Tools** MSBuild toolset (`ZigCompile`,
`ZigLd`, `ZigAr`, driven from `Rxdk.MsBuild`) used by VS20XX, and the equivalent CLI/VS Code path.

Work:
- Point `ZigCompile` at `clang --target=i686-pc-windows-gnu` and `ZigLd` at
  `clang … -fuse-ld=lld` (or `ld.lld`), `ZigAr` at `llvm-ar`/`llvm-lib` — mirroring the
  RXDK-Libs Phase-1 swap. (These tasks were just re-based on version-stable
  `Microsoft.Build.Utilities` and build their command lines explicitly, so the exe/flag swap is
  contained.)
- Supply **compiler-rt builtins** for `i686-windows-gnu` — this becomes **required** here (the
  title link is what pulls them in; zig currently auto-provides them). Add `compiler-rt`
  (builtins) to the xboxog LLVM CI (`LLVM_ENABLE_RUNTIMES`/`PROJECTS`) cross-built for i386,
  packaged as `libclang_rt.builtins-i386.a`; or compile the needed builtins from the vendored
  `vendor/llvm-project/compiler-rt` source (we already carry it).
- Re-validate the `libcompat` override still wins over the new compiler-rt (the ABI-mismatched
  `fabs`/weak SSE2 `memmove` reasons in `build.ps1` still apply).
- Ship the toolchain (clang/lld/llvm-lib/llvm-ar + builtins) the same way host tools ship today
  (download the `xboxog-<os>-<arch>.zip` from the release into the staged tools root), so both
  VS20XX and VS Code drive one LLVM toolchain.

## Open questions / risks
- **llvm-lib packaging** — needs a one-line CI change on the `xboxog` branch's build workflow.
- **Version parity** — zig's clang vs xboxog LLVM; affects byte-identical reproducibility.
- **`-target x86-windows-gnu` vs `--target=i686-pc-windows-gnu`** — confirm clang defaults (data
  layout, `__GNUC__`, wchar) match what zig cc produced; RXDK's `-nostdinc`+own headers minimise
  the surface but the builtin predefines still matter.
- **`.s`/`.asm`** — `.asm` is skipped in `compile_c`; `.s` goes through `cc` (clang integrated asm) — carries over.
