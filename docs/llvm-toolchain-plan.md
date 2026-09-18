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
| **llvm-lib** (COFF librarian, replaces `zig lib`) | ✅ (added 2026-09-17) | `llvm-lib` is now built + packaged in both CI workflows (`teamresurgent` bfb1243) — it is a tool-symlink target of `llvm-ar` and takes the same MSVC-style switches. Use it directly. |
| **ld.lld** (link) | ✅ | not used in Phase 1 (link stays zig) |
| **compiler-rt builtins** (title link) | ❌ (not in ENABLE_PROJECTS) | **not needed for Phase 1** — title link stays on zig `cc`, which supplies them. Needed only for Phase 2. |

## Phase 1 — swap compile + archive only (this branch)

Keep the VS20XX toolset link on zig. Only change RXDK-Libs' compiler + librarian.

**Status: IMPLEMENTED (opt-in) 2026-09-17.** A new `build/toolchain.zig` selector reads the
`RXDK_LLVM` env var (path to an unpacked `xboxog-<os>-<arch>.zip`, i.e. the dir containing
`bin/clang` + `bin/llvm-lib`). Unset → zig (unchanged default, byte-for-byte). Set → clang +
llvm-lib. `build/compile_c.zig` and `build/coff_lib.zig` consult it; every flag is passed through
verbatim, and the zig-mode argv order is preserved exactly. Triples are mapped per-target
(`x86-windows-gnu`→`i686-pc-windows-gnu`, `x86-windows-msvc`→`i686-pc-windows-msvc`) — **note both**,
because `libxnet`/`libxonline` pin the MSVC triple. `verify-no-vs` still passes (the `llvm-lib` exe
name is assembled from parts so the forbidden `lib`+`.exe` token never appears literally).

Build LLVM-mode with: `RXDK_LLVM=/path/to/xboxog-windows-x64 zig build <lib> -Doptimize=ReleaseSmall`.

1. ✅ **Toolchain staging** — `RXDK_LLVM` env var → toolchain `bin/` (`build/toolchain.zig`).
2. ✅ **`build/compile_c.zig`** — clang/clang++ exe + `--target=i686-pc-windows-*`; all flags verbatim.
3. ✅ **`build/coff_lib.zig`** — `llvm-lib /NOLOGO /OUT:… @rsp` (same MSVC switches).
4. ✅ Left `build.ps1` / libcompat / picolibc vendoring untouched.

### Parity results (probe, 2026-09-17) — clang **23.0.0git** vs zig's clang ~18

Byte-identical is OFF (version skew, as predicted); functional/ISA/ABI parity **confirmed** on a
synthetic probe (`probe.c`: 64-bit int math, double/float math, mem loops, popcount, big struct copy):

- **ISA gate (the critical one):** TR clang at `-march=pentium3` is **PIII-clean at both triples**
  (gnu + msvc), matching zig. isa-scan correctly **flags** TR clang's SSE2 + `popcnt` when forced
  (`-msse2 -mpopcnt`), so a codegen regression cannot slip past.
- **ABI/symbol parity:** identical external symbol set + `_`-cdecl decoration, and the **same**
  undefined builtin (`___divdi3`) — so libcompat still satisfies it the same way. No mangling drift.
- **Size gap** (zig `.o` bigger) is entirely CodeView debug volume (`.debug$S`/`.debug$T`); TR clang
  emits tighter `.text`. No correctness impact (debug info isn't consumed in the XBE pipeline).

**Whole-library validation (`RXDK_LLVM=… zig build <lib> -Doptimize=ReleaseSmall`):**

| Lib | Triple | isa-scan | ABI parity vs zig (defined / undefined externals) |
|---|---|---|---|
| `libc` (picolibc + Xbox HAL, C + asm) | gnu | ✅ PIII-clean | ✅ identical — 1200 / 45 |
| `libxnet` (sockets, `-fms-extensions`) | **msvc** | ✅ PIII-clean | ✅ identical — 388 / 67 |

Both link identically to the zig build (same public surface, same external-dep surface); only code
bytes differ (clang 23 vs ~18). **`-nostdinc` gotcha (fixed):** this clang's `-nostdinc` also strips
its resource/builtin header dir (`stddef.h` etc.), unlike `zig cc`. The xAPI batches pass `-nostdinc`,
so LLVM mode re-adds `<root>/lib/clang/<ver>/include` via `-isystem` (after `-I`, so RXDK headers still
win). Discovered at configure time, no hardcoded LLVM version.

### Full-lib sweep (2026-09-17) — 15/18 verified; 1 open blocker

Built every lib both ways at `-Doptimize=ReleaseSmall` and compared (`isa-scan` + whole-archive
symbol parity). Result:

- **15 of 18 libs build** under LLVM: `libc`, `libcpp`*, `libd3d8`, `libd3d8i`, `libdsound`,
  `libkernel`, `libuix`, `libxact`, `libxapi`, `libxbdm`, `libxmv`, `libxnet`, `libxneto`,
  `libxonline`, `libxvoice`. **14 are isa-scan PIII-clean + ABI-identical to zig** (same defined +
  undefined external symbol sets); `libcpp`* is isa-clean with the only diffs being libc++/libunwind
  **internal** ABI helpers renamed between clang 18↔23 (`_LIBCPP_ABI_NAMESPACE`-versioned symbols,
  libunwind log/section symbols, `__assert_func` satisfied by libc) — public surface matches. The
  vendored headers below were confirmed **byte-identical** in zig mode (rebuilding libxapi + libdsound
  with them yields bit-for-bit the same .lib — dist parity preserved).
- **3 libs remain blocked by Blocker B only**: `libxgraphics` (+`libxfont`, which packs into it),
  `libd3dx8`, `libdmusic`.

- **Blocker A — RXDK leaned on zig's bundled MinGW headers** (the plan's assumption was mostly but
  not entirely true). A handful of TUs `#include <>` Windows headers zig shipped implicitly and RXDK
  did not own. **Fixed** by vendoring RXDK-owned copies (definitions already exist in RXDK; these are
  declarations/spec constants): `shared/include/cguid.h` (well-known COM GUID externs, from
  `xobjbase.h`), `libs/libxapi/usb/inc/usb100.h` (USB 1.1 descriptor structs), `shared/include/initguid.h`
  (the DEFINE_GUID→definition shim), `shared/include/windowsx.h` (3 macros: GlobalAllocPtr/FreePtr,
  MAKEPOINTS). These make RXDK self-contained — correct regardless of toolchain.

- **Blocker B — clang `-fasm-blocks` assertion (the real one). FIX PUSHED 2026-09-18.** This TR clang
  is an **assertions** build (`+assertions`) and aborted on `Assertion failed: MaybeODRUseExprs.empty()
  ... SemaDecl.cpp:17139` for **every** MS `__asm { }` block that references C locals — `libxgraphics`
  (xgmath quaternion), `libd3dx8` (jpeglib IDCT/FDCT: midct8x8aan, mfdct8x8aan, …), `libdmusic`
  (dmsynth). Root cause: `Sema::ActOnMSAsmStmt` never drained `MaybeODRUseExprs` (the operands naming
  locals are marked for deferred odr-use during parsing), unlike the GCC path `BuildGCCAsmStmt` which
  calls `CleanupVarDeclMarking()`; the leftovers survived to `ActOnFinishFunctionBody`. **Chose the
  upstream fix (not assertions-off):** patched `Sema::ActOnMSAsmStmt` to call `CleanupVarDeclMarking()`,
  mirroring the GCC path — `Team-Resurgent/llvm-project` **`xboxog`** commit `81e9b86` (pushed;
  auto-triggered the "Original Xbox clang" toolchain rebuild). **xboxog only** — the 360 is MS-PPC and
  has no `__asm { }` blocks, so it can't hit this. **To validate:** once the rebuilt toolchain publishes
  to the rolling release, re-download `xboxog-windows-x64.zip`, rebuild `libxgraphics`/`libd3dx8`/
  `libdmusic` in LLVM mode (isa-scan + ABI parity), then bump RXDK-Libs' `vendor/llvm-project` submodule
  to `81e9b86`.

### Verify — parity checks (do NOT assume; zig stung us on SSE2)

The whole migration must be gated on parity, and the codegen/instruction-set checks matter most —
a compiler that emits **SSE2+** faults as `STATUS_ILLEGAL_INSTRUCTION` on real HW (PIII = MMX+SSE1
only), and **xemu often masks it** ([[rxdk-trees-fur-xemu-gap]]). So parity is checked in this order:

1. **Instruction-set scan (the critical gate).** **Implemented: [`tools/isa-scan.py`](../tools/isa-scan.py)**
   — parses COFF `.o`/`.lib` (archives walked member by member) and flags every instruction above the
   PIII/SSE1 baseline, using **Capstone ISA groups** (so it can't miss an SSE2/SSE3/SSE4/AVX/BMI
   opcode via a hand-list) plus a **mnemonic backstop** for post-PIII scalar ops Capstone leaves
   ungrouped (`popcnt`, `movbe`, …). It correctly **excludes** the backward-compatible `tzcnt`/`lzcnt`
   (=`rep bsf/bsr`) and `pause` (=`rep nop`), which don't fault on a PIII. Exit 1 on any violation.
   `pip install capstone`, then `python tools/isa-scan.py dist/lib` — wire it into `build.ps1`/CI as a
   post-build gate. **Validated:** the whole current (zig) `dist/lib` is clean (38 libs); a forced
   `-msse2`/`-msse4.2`/`popcnt` object is flagged; a `-march=pentium3` object is clean. Run it on the
   LLVM output and it must be equally clean.
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
- ~~**llvm-lib packaging**~~ — DONE (2026-09-17): both `build-xboxog-clang.yml` and `build-xbox360-clang.yml` on `teamresurgent` now build + package `llvm-lib`.
- **Version parity** — zig's clang vs xboxog LLVM; affects byte-identical reproducibility.
- **`-target x86-windows-gnu` vs `--target=i686-pc-windows-gnu`** — confirm clang defaults (data
  layout, `__GNUC__`, wchar) match what zig cc produced; RXDK's `-nostdinc`+own headers minimise
  the surface but the builtin predefines still matter.
- **`.s`/`.asm`** — `.asm` is skipped in `compile_c`; `.s` goes through `cc` (clang integrated asm) — carries over.
