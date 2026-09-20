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

### Full-lib sweep — 18/18 build under LLVM, all PIII-clean (2026-09-18)

After the clang Sema fix (Blocker B) + vendored headers, **all 18 libs build under LLVM and are
isa-scan PIII-clean.** ABI vs zig: **14 identical**; **4 differ only by benign clang-23-vs-18 weak-symbol
elision** — LLVM emits strictly *fewer* symbols (never new/divergent): `libcpp` (libc++/libunwind
internals), `libxgraphics`/`libdmusic` (unused `___udivdi3` builtin optimized away), `libd3dx8`/`libdmusic`
(unused implicit C++ dtors/vtables/template instantiations — `XSource`, `SmartRef::ComPtr<T>` — which
are weak/COMDAT and re-emitted wherever actually used). Final link-level validation is the sample/HW
stage. The 3 `__asm` libs (`libxgraphics`, `libd3dx8`, `libdmusic`) were validated against the rebuilt
clang (xboxog `81e9b86`); a local `llvm-lib` was aliased from `llvm-ar` (see llvm-lib CI note below).

### Earlier snapshot (2026-09-17) — 15/18 verified; 1 open blocker

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

### Phase 2 status — engine LLVM path landed (opt-in), HW-validated (2026-09-19)

The shared **.NET engine** (`Rxdk.Engine`, used by BOTH VS Code and VS20XX and the CLI — this is
the real title compile/link path, not the older MSBuild `ZigCompile`/`ZigLd`/`ZigAr` tasks) now
compiles, archives, and links titles with **either** backend behind a `Toolchain` abstraction:

- **`Build/Toolchain.cs`** — the zig-vs-LLVM abstraction. Per-backend: executable, target triple
  (`x86-windows-gnu` vs `i686-pc-windows-gnu`), the compile/archive/link **subcommand token** zig
  needs and clang does not (`cc`/`c++`/`ar`), the resource-include the `-nostdinc` clang build must
  re-add, the link runtime-lib arg (`-rtlib=compiler-rt` vs `-fuse-ld=lld`), and the compiler-rt
  builtins archive. **LLVM is opt-in:** `Toolchain.ResolveAsync` picks LLVM when it resolves
  (`RXDK_LLVM` env / managed install), else zig.
- **`Bootstrap/LlvmRuntime.cs`** — locates the xboxog clang root, its `llvm-ar`, the versioned
  resource-include dir, and `libclang_rt.builtins-i386.a`.
- **`Build/XdkLink.cs` / `Build/XboxBuild.cs`** — thread `Toolchain` through compile / `.eh_frame`
  brackets / archive / link. The link appends the builtins archive after the SDK libs (LLVM only).

**Two self-containedness gaps closed** (both were masked by `zig cc`'s bundled MinGW headers /
compiler-rt, exactly the Phase-1 Blocker-A/compiler-rt hazards):

1. **MinGW pack headers** — the public SDK headers `#include <pshpack{1,2,4,8}.h>` / `<poppack.h>`
   but those were never exported to `shared/include`; `zig cc` silently supplied MinGW's copies.
   Fixed by exporting RXDK's own clean-room copies (already in `libs/libxapi/nt/`) to
   `shared/include/` so the SDK is toolchain-independent.
2. **compiler-rt 64-bit builtins** — `__divdi3/__udivdi3/__moddi3/__umoddi3` (+ shift/mul/cmp and
   int↔fp conversion helpers) are referenced by libcompat's MSVC `__alldiv` shim and picolibc's
   `__ultoa_invert`; `zig cc` auto-linked them from its compiler-rt. Built into
   `libclang_rt.builtins-i386.a` from the vendored `compiler-rt/lib/builtins` source (subset:
   pure integer/fp math, no OS/mem/emutls builtins that would collide with the RXDK libs) via
   **`tools/build-rt-builtins.ps1`**, PIII-clean (isa-scan), placed at the toolchain's canonical
   `lib/clang/<ver>/lib/windows/` where the engine finds it. libcompat's whole-archive
   `fabs`/`memmove` still win — the builtins archive is pulled on demand, not whole-archive.
   The llvm-project submodule sparse checkout was extended with `compiler-rt/lib/builtins`.

**Validation — 7 samples booted on real HW (192.168.1.134), all isa-scan PIII-clean:**
`Tut01_CreateDevice` (C++, libc++ + `.eh_frame` brackets), `BackgroundMusic` (libdsound+libdmusic),
`QualityOfService` (libxnet/libxneto, MSVC-triple libs), `SimpleXMV` (libxmv), `CustomSTLAllocators`
(libc++ — allocator tests ran to completion + clean exit), `Dolphin` **Release/-Os** (libd3dx8/
libxgraphics `__asm` libs, ~330 fps, visually confirmed), `Gamepad` (libxapi). No
`STATUS_ILLEGAL_INSTRUCTION`. The sweep surfaced ONE gap beyond the 64-bit divide builtins: **`__alloca`**
(the i386 stack-probe `alloca()` lowers to — the Common sample helpers use `alloca`, Tui01 didn't),
supplied by zig's mingw runtime. Fixed by adding compiler-rt `i386/chkstk.S` (`__chkstk_ms`) +
`i386/chkstk2.S` (`_alloca`/`__chkstk`) to the builtins archive. zig remains the default; nothing
changes unless `RXDK_LLVM` is set.

**Shipping + auto-install — DONE 2026-09-19 (RXDK-Tools `1e81029`).** `LlvmInstaller` downloads the
per-host `xboxog-<os>-<arch>.zip` from the `Team-Resurgent/llvm-project` rolling release (tag
`latest`) into the managed root (`%LocalAppData%/RXDK/llvm` on Windows), the same way host tools
ship. Host OS/arch selects the asset, covering all 6 variants VS Code needs (incl. `windows-arm64`);
VS20XX is Windows-only. New CLI: `install-llvm` / `update-llvm` (`--tag`) / `llvm-status`. Crucially,
**SELECTION is gated separately from LOCATION**: `Toolchain.ResolveAsync` picks LLVM only on opt-in
(`RXDK_LLVM` path / `RXDK_USE_LLVM=1` / explicit override) — a managed install merely being present
does NOT flip a build off zig, so installing the toolchain to try it is safe. Verified: staged
toolchain + plain build → Zig; `RXDK_USE_LLVM=1` → LLVM from the managed install, no path needed.

**CI packaging — DONE 2026-09-19 (llvm-project `xboxog` `a0da7cf7a`; build in progress).** The
`Original Xbox clang` workflow now builds the CANONICAL compiler-rt builtins with the freshly built
clang and ships `libclang_rt.builtins-i386.a` inside each zip at `lib/clang/<ver>/lib/windows/`,
where the engine links it. Key details (recipe validated locally against the current xboxog clang
before pushing): add `compiler-rt` to the checkout cone; build `llvm-nm/llvm-ranlib/llvm-objdump/
llvm-rc`; standalone builtins build cross to `i686-pc-windows-gnu` (`CMAKE_SYSTEM_NAME=Windows` +
`llvm-rc`; `TRY_COMPILE` static-lib; `COMPILER_RT_BAREMETAL_BUILD` + a 1-decl `stdlib.h` stub for
`int_util.c`'s `_WIN32`-guarded include). **PIII gotcha discovered + handled:** `-march=pentium3`
pins the generic `.c` codegen to x87, but compiler-rt still prefers its i386 `.S` asm for 6
int→float conversions (`float{,un}di{df,sf,xf}`) whose hand-written asm uses SSE2 (`movsd xmm`)
regardless of `-march` — so the step replaces those 6 objects with the generic `.c` compiled at
pentium3, and an `llvm-objdump` tripwire asserts the whole archive is SSE2/AVX-free (RXDK-Libs
`isa-scan.py` is the authoritative Capstone gate downstream). Once this publishes, `install-llvm`'s
missing-builtins warning goes away and `tools/build-rt-builtins.ps1` becomes a dev fallback.

**Remaining tail (to make LLVM the default / drop zig):** (a) confirm the CI build publishes a
builtins-carrying zip and re-validate an `install-llvm` → LLVM build end-to-end from the shipped
toolchain; make the `compiler-rt/lib/builtins` sparse-checkout durable for local dev; (b) then flip
the `Toolchain.ResolveAsync` opt-in gate to make LLVM the default and retire zig.

## Open questions / risks
- ~~**llvm-lib packaging**~~ — DONE (2026-09-17): both `build-xboxog-clang.yml` and `build-xbox360-clang.yml` on `teamresurgent` now build + package `llvm-lib`.
- **Version parity** — zig's clang vs xboxog LLVM; affects byte-identical reproducibility.
- **`-target x86-windows-gnu` vs `--target=i686-pc-windows-gnu`** — confirm clang defaults (data
  layout, `__GNUC__`, wchar) match what zig cc produced; RXDK's `-nostdinc`+own headers minimise
  the surface but the builtin predefines still matter.
- **`.s`/`.asm`** — `.asm` is skipped in `compile_c`; `.s` goes through `cc` (clang integrated asm) — carries over.
