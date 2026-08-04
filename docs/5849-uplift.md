# RXDK-Libs → XDK-5849 ABI uplift status

The RXDK libs were ported from the **May-2020 Xbox source leak**, whose library sources date to
around **January 2002**. The XDK the samples target is **5849.17** (a later build), so the public
APIs the samples call have drifted from what the leak implemented (struct reshuffles, added enums,
reversed argument orders, new functions). This doc tracks, per lib, whether its source has been
reconciled to the 5849 ABI.

## Method

We do **not** decompile wholesale. For each lib:

1. **Extract ground-truth types** from the 5849 prebuilt retail `.lib` with `cvdump`
   (`RXDK-Tools/src/Rxdk.CvDump`). It reads the CodeView `.debug$T` sections carried in the COFF
   objects and renders every struct/union/enum with exact byte offsets, sizes, and enum values —
   no disassembly.
   ```
   cvdump <5849-lib> [NameFilter ...]
   ```
2. **Diff** the recovered 5849 layouts against the leak's internal headers.
3. **Apply the delta** to the RXDK source (struct layout, enum values, signatures, arg order) and
   adopt the XDK-5849 public header for the samples.
4. **Verify** by diffing the rebuilt lib's disassembly against the 5849 retail `.lib` for the
   touched functions (the same byte-exact-oracle approach used for the bundler), and re-run the
   affected samples.

Only genuinely-new *behavior* (a function the leak lacks entirely) needs targeted disassembly of
the 5849 retail lib, guided by the leak source as reference.

**Only the plain retail `.lib` is authoritative.** Ignore the `*ltcg.lib` (link-time codegen),
`*i.lib` (instrumented / CAP-profile), and `*d.lib` (debug) variants — the retail lib already
carries full `.debug$T`.

## Status legend

- ❌ **leak** — still at Jan-2002 leak ABI; not yet reconciled.
- 🔶 **in progress** — reconciliation underway.
- ✅ **5849** — reconciled to 5849 ABI and verified.
- ➖ **n/a** — no meaningful 5849 drift (or no direct 5849 counterpart).

## Matrix

| RXDK lib | 5849 retail lib | Status | Notes |
|---|---|---|---|
| libxact | `xacteng.lib` | ✅ 5849 | Full notification/marker ABI reconciled + the 5849 API surface for the sample suite. Reshape adapters: `PlayEx`, `PrepareEx`, `RegisterStreamedWaveBank`, `SetMasterVolume(+wCategory)`. No-op stubs for genuinely-new-in-5849 subsystems the leak lacks: runtime `SetVariable`/`GetVariable`, `SetParameterControl`, `SetListenerPosition/Velocity/Orientation`. Removed the leak's global `operator new/delete`. **Verified: ALL 5 XACT samples build to bootable .xbe** (compile+link+imagebld). HW audio test pending; stubbed features (streaming/WMA/param-control/3D-listener) are inert by design. |
| libxonline | `xonline.lib` (+ `xonlinen`/`xonlines`, `uix.lib`) | 🔶 partial | 5849 public `xonline.h` adopted; leak client compiles+links on libxnet(LIBO). `src/uplift5849.cpp` carries the documented-stub tail (stats/storage/friends/signature/silent-logon/matchmaking/mutelist/messaging/arbitration/competition/teams/presence/query — each fails cleanly, no task). `src/uix5849.cpp` stubs the **UIX drop-in UI** (`uix.lib` folded here): `UIXCreateLiveEngine/UIPlugin/AudioPlugin` return a real refcounted no-op engine, features never start (no UI is drawn). All must be implemented for real for Insignia. |
| libxvoice | `xvoice.lib` | ✅ 5849 | **Fresh implementation, NOT a leak port** — the leak has no xvoice source at all (only an older public header; codec + USB audio driver were binary-only). Adopted 5849 `xhv.h`/`xvoice.h`; XHV engine (`XHVEngineCreate` + 27 C exports + `_xhv_*_mode` globals) with REAL talker/mode/callback bookkeeping; communicator/codec paths faithfully report no-headset (`GetLocalTalkerStatus`=REMOVED, `IsTalking`=FALSE, packets accepted+discarded, VoiceMail/SR fail cleanly). Low-level `XVoiceCreate*` XMO factories fail as with no communicator (SC03 codec is binary-only in the retail lib). Exports `XDEVICE_TYPE_VOICE_{MICROPHONE,HEADPHONE}_TABLE` (zeroed → `XGetDevices` sees none). All 5849 stdcall decorations verified against the retail lib. |
| libdmusic | `dmusic.lib` | ❌ leak | Largest COM surface. Assess drift once XACT proven. |
| libd3d8 | `d3d8.lib` | ❌ leak | Samples want a few newer D3D constants (`D3DRS_DEPTHCLIPCONTROL`, `D3DDevice_GetBackBuffer2`, …). |
| libd3dx8 | `d3dx8.lib` | ❌ leak | |
| libxgraphics | `xgraphics.lib` | ❌ leak | |
| libdsound | `dsound.lib` | 🔶 partial | Added `DSVOICEPROPS` (5849, recovered via cvdump) — needed by the 5849 xact.h. Added (header-only) `DSBCAPS_FXIN2`, `DSMIXBIN_VOICE_0..3`, `DSMIXBINVOLUMEPAIRS_{REQUIRED,DEFAULT}_5CHANNEL_3D[_PLUS_LFE]` for the voice/Marketplace samples. Still: a few newer constants (`DSBPLAY_SYNCHPLAYBACK`…). |
| libxapi | `xapilib.lib` | 🔶 partial | Added the 5849 `XSaveFloatingPointStateForDpc`/`XRestoreFloatingPointStateForDpc` (k32/fpudpc.c — single static `KFLOATING_SAVE`; DPCs are serialized on the single-CPU Xbox). Rest unassessed. |
| libxnet | `xnet.lib` (+ `xnetn`/`xnets`) | ❌ leak | `INVALID_SOCKET`/`XNetGetConnectStatus` glue for the Live samples lives here. |
| libxmv | `xmv.lib` | ❌ leak | |
| libc | `libc.lib` / `libcmt.lib` | ➖ n/a | picolibc-based; not a 1:1 XDK CRT. |
| libcpp | `libcp.lib` / `libcpmt.lib` | ➖ n/a | |
| libkernel | `xboxkrnl.lib` | ❌ leak | Kernel import surface; drift low-risk but check. |
| libxbdm | `xbdm.lib` | ❌ leak | Debug monitor. |
| libxfont | *(none)* | ➖ n/a | No standalone `xfont.lib` in 5849; font lives in xapi/xgraphics. |

## Not yet ported (5849 libs with no RXDK counterpart)

| 5849 lib | What it is | Disposition |
|---|---|---|
| `xvoice.lib` | XHV — Xbox high-level voice chat | **PORTED — see `libxvoice` in the matrix above.** (The leak had no source; fresh implementation of the 5849 public surface.) |
| `xsndtrk.lib` | Soundtrack (user music) | Dropped in importer LibMap (`xsndtrk=null`). |
| `xperf.lib` | PIX perf capture | Binary-only, dropped (`xperf=null`). |
| `uix.lib` | Xbox UI helper (Live) | Folded into libxonline (`src/uix5849.cpp`): creation + frame loop are real no-ops; features (logon/friends/players UI) never start. Real port needed for Insignia. |
| `oldnames.lib` | MSVC POSIX-name aliases | n/a. |

## Cross-cutting issues surfaced by the libxact uplift (watch for these on every lib)

Getting the first lib to actually **link** (not just compile) exposed several latent issues that
will recur:

1. **Cross-lib type cascades.** The 5849 public `xact.h` referenced `DSVOICEPROPS`, forcing a
   partial dsound uplift. Expect adopting one lib's 5849 header to pull types from its dependencies
   — recover them with `cvdump <dep>.lib <TypeName>` and add them to the dependency's header.
2. **Global `operator new/delete` in a lib.** libxact shipped a global replacement (leak artifact);
   5849's retail lib exports none and uses the CRT's. A lib providing its own collides at link
   (duplicate symbol). Check `dumpbin /LINKERMEMBER:1 <5849lib> | grep '??[23]@'` — if empty, the
   lib should NOT define global new/delete.
3. **Renamed/reshaped exports.** 5849 turned `IXACTSoundBank_Play` (5 args) into a header-inline
   over a new exported `IXACTSoundBank_PlayEx` (packs `XACT_PREPARE_SOUNDCUE`). When the samples'
   inline wrappers reference a symbol the leak lib lacks, add a thin adapter export.
4. **Stale deployed SDK.** The SDK under `C:\ProgramData\RXDK\sdk\lib\{debug,release}` was months
   stale (missing `_vsnprintf` etc.). Re-deploy the current `zig-out/lib` before link-testing.
5. **Stale imported sample vcxprojs.** Samples imported before a LibMap change carry an outdated
   `<RxdkLibraries>` (missing the middleware lib). Re-import (or patch) so the manifest links it.

Additional cross-cutting issue surfaced by the voice-sample sweep:

6. **MSVC `__inline` member definitions in a .cpp.** Several leak classes define members
   `__inline` in one TU while other TUs call them (e.g. libxact's `CEngine::AddRef` in
   engine.cpp, called from soundbank.cpp; the LowLevelVoiceChat sample's
   `CVoiceCommunicator::Submit*Packet`). MSVC emitted them anyway; standard C++ emits no
   out-of-line copy, so the cross-TU reference dies at link. Fix: drop the `__inline`.
   Related MSVC-ism: `friend` declarations don't inject the name at namespace scope
   (sample callbacks needed real forward declarations).

_Last updated: 2026-08-04. Tooling: `cvdump` (RXDK-Tools). See also the samples/middleware memory note._
