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
| libxonline | `xonline.lib` (+ `xonlinen`/`xonlines`, `uix.lib`) | ✅ 5849* | 5849 public `xonline.h` adopted; leak client compiles+links on libxnet(LIBO). **The UIX drop-in UI (`src/uix5849.cpp`) is fully functional** over the leak's real Live client: **logon** (multi-user 1–4 per-controller claims, guest sign-in, passcode entry, SILENT / RETRIEVED_STATE / RETRIEVED_GAME_INVITE starts, real `XOnlineLogon` task pumped in `DoWork`, connection held open after success), **friends** (real enumeration; accept/decline requests+invites, join, remove, invite-to-game, sign-out; `FRIENDS_JOIN_GAME[_CROSS_TITLE]` exits with the `XONLINE_FRIEND`), **players** (real `ILivePlayersList` registry + screen + mute menu + selection info), **notifications** (real, friends-state derived, property-gated), **`ILiveFriendsList`**, and a real `Reboot` (`XLaunchNewImage` to the dashboard). Also real: `XOnlineSilentLogon`, `XOnlineChangeLogonUsers` (re-logon with captured services), `XOnlineSave/RetrieveLogonState`, `XOnlineGetNotification` (friends-state derived), `XOnlineTitleIdIsSameTitle` (XBE cert). Rendering is the font-renderer contract by design (the retail skin pipeline is unbuildable on modern hosts). *Remaining asterisk: the 5849-new LSP services (stats v2/storage/messaging/arbitration/competition/teams/mutelist/offerings/query and peer QoS probes) report service-unavailable — a **protocol boundary** (wire formats absent from the leak), not open work. |
| libxvoice | `xvoice.lib` | ✅ 5849 | **Fresh implementation, NOT a leak port** — the leak has no xvoice source at all (only an older public header; codec + USB audio driver were binary-only). Adopted 5849 `xhv.h`/`xvoice.h`; XHV engine (`XHVEngineCreate` + 27 C exports + `_xhv_*_mode` globals) with REAL talker/mode/callback bookkeeping; communicator/codec paths faithfully report no-headset (`GetLocalTalkerStatus`=REMOVED, `IsTalking`=FALSE, packets accepted+discarded, VoiceMail/SR fail cleanly). Low-level `XVoiceCreate*` XMO factories fail as with no communicator (SC03 codec is binary-only in the retail lib). Exports `XDEVICE_TYPE_VOICE_{MICROPHONE,HEADPHONE}_TABLE` (zeroed → `XGetDevices` sees none). All 5849 stdcall decorations verified against the retail lib. |
| libdmusic | `dmusic.lib` | ✅ samples green | **All 7 DirectMusic samples build.** Fixes: the sample projects were never listing libdmusic at all; duplicate `GUID_NULL` and two `ReadMBSfromWCS`; `heap_rtl.h` missing its `extern "C"` (HeapAlloc resolved to a C++-mangled `RtlAllocateHeap`); and **`dsoundsequencer.cpp` now builds** in its own driver-env slice — it needs libdsound's bridge + internal includes, because dsoundi.h's kernel typedefs redefine the title-side ones, and the file is entirely `#ifdef XMIX`. Its two interfaces also had `Clear`/`ClearAtOrAfter` declared without `PURE` (the only such methods), so no vtable was ever emitted. Not HW-tested — no MIDI/DLS playback heard. |
| libd3d8 | `d3d8.lib` | ✅ 5849 | Full ABI reconciliation. Headers: 5849 `D3DRENDERSTATETYPE` adopted verbatim (three append-only shifts — simple +10, deferred +19, complex +20), all 47 new types/defines, the three header-embedded dispatch tables replaced wholesale, and the declaration / inline-dispatch / object-struct / wrapper regions lifted from 5849. Runtime: `g_RenderStateEncodings` +10 (new states use the same parameterized-NOP method the leak already used for its reserved slot), complex table + `SetRenderState_SampleAlpha`, `g_InitialRenderStates` extended at all three boundaries. **44 new exports** in `se/uplift5849.cpp` (the `*2` return-pointer adapters, 5 `SetVertexShaderConstant` variants, and real stipple / depth-clip-plane / SAMPLEALPHA / push-distance / wait+timer-callback implementations). Two ABI changes propagated into the lib: `BeginPush` returns the pointer, `CreatePixelShader` is void. The 5849 `BeginState`/`EndState` direct-push contract works via a linker alias of `D3D__Device` onto `g_Device` (whose pusher is already its first member by design). `D3DVERTEXBUFFER_ALIGNMENT` 4→1. **Verified: all 43 previously-green samples still build, zero regressions**; all dependent libs (d3dx8/xgraphics/dsound/xact/xonline/xvoice/xmv) rebuild clean. Not HW-tested. |
| libd3dx8 | `d3dx8.lib` | ❌ leak | |
| libxgraphics | `xgraphics.lib` | ❌ leak | |
| libdsound | `dsound.lib` | ✅ 5849 | Added `DSVOICEPROPS` (5849, recovered via cvdump) — needed by the 5849 xact.h. Added (header-only) `DSBCAPS_FXIN2`, `DSMIXBIN_VOICE_0..3`, `DSMIXBINVOLUMEPAIRS_{REQUIRED,DEFAULT}_5CHANNEL_3D[_PLUS_LFE]` for the voice/Marketplace samples. **Three runtime features implemented, each recovered by disassembling the retail lib rather than guessed:** (1) `DSBPLAY_SYNCHPLAYBACK` — `Play` arms the voice on but *paused* (register `0x140` is `NV1BA0_PIO_VOICE_PAUSE`, ACTION bit 18, `RESUME`=0), tags it `SYNCHPENDING` and bumps a counter; `IDirectSound_SynchPlayback` then resumes every pending voice inside one `VOICE_LOCK` at raised IRQL so they start on the same sample. (2) `DSBCAPS_MUTE3DATMAXDISTANCE` — distance volume becomes `DSBVOLUME_MIN` at/beyond `MaxDistance`; latched into `DS3DSOURCEPARAMS` at 3D-param allocation since it is creation-time. (3) **The WMA decoder XMO** — the leak has no WMA support at all. The ported ffmpeg WMA v1/v2 decoder moved here from libxact (matching 5849, where `dsound.lib` owns it and `xact.lib` calls in), joined by a new ASF reader (`wma/asf.c`: header-object walk + data-packet parsing incl. multiple/compressed payloads) and a packet-at-a-time front end (`wma/wmastream.c`), under `CWmaMediaObject` (`dsound/wmaxmo.cpp`) and the five 5849 factories. `XWmaDecoderCreateMediaObject` defers the header read to `DoWork` and withholds `ACCEPT_OUTPUT_DATA` until it lands — the handshake the streaming samples poll on. Header ABI: 5849's `DoWork` on `XFileMediaObject`/`XWaveFileMediaObject` and `DoWork`+`SeekToTime` on `XWmaFileMediaObject`, adopted in both `dsound.h` and `dsoundp.h`. **Verified: MultipleListeners, SetRolloffCurve, WMAStream, WMAInMemory, EnumSoundtrack, BackgroundMusic and FastLoad all build to bootable .xbe.** Not HW-tested. |
| libxapi | `xapilib.lib` | 🔶 partial | Added the 5849 `XSaveFloatingPointStateForDpc`/`XRestoreFloatingPointStateForDpc` (k32/fpudpc.c — single static `KFLOATING_SAVE`; DPCs are serialized on the single-CPU Xbox). Rest unassessed. |
| libxnet | `xnet.lib` (+ `xnetn`/`xnets`) | ❌ leak | `INVALID_SOCKET`/`XNetGetConnectStatus` glue for the Live samples lives here. |
| libxmv | `xmv.lib` | ✅ exports | **Full retail export parity.** The port implemented the `GetNextFrame` path (title drives decoding and presents each frame); the remaining 13 exports are now in: `Play` (blocking whole-movie playback through the D3D overlay, so it can run on a background thread without disturbing the title's own rendering), `CreateDecoderForPackets` (drains the title's callbacks into one image up front — the demuxer works over a complete in-memory image, as `CreateDecoderForFile` already did), `Reset`, `Terminate{Loop,Playback,Immediately}`, `Get/SetSynchronizationStream`, `GetAudioStream`, `DisableAudioStream`, `GetTimeFromStart`, `g_XMVInhibitDebugOutput`, `XMVBuildNumber`. Header quirks worth knowing: `Play`'s prose names `XMVPLAY_LOOP`, which xmv.h never defines (`XMVFLAG_FULL_LOOP` is the real bit); `GetAudioStream` returns a **borrowed** pointer with no AddRef, matching `EnableAudioStream`. **Verified: SimpleXMV, XMVCompare, XMVPlayer all build to bootable .xbe.** Video decode is still the port's placeholder for unsupported geometry. |
| libc | `libc.lib` / `libcmt.lib` | ➖ n/a | picolibc-based; not a 1:1 XDK CRT. |
| libcpp | `libcp.lib` / `libcpmt.lib` | ➖ n/a | |
| libkernel | `xboxkrnl.lib` | ❌ leak | Kernel import surface; drift low-risk but check. |
| libxbdm | `xbdm.lib` | ❌ leak | Debug monitor. |
| libxfont | (inside `xgraphics.lib`) | ✅ full | **Full export parity (30/30).** Was bitmap-fonts-from-memory only; TrueType (`truetype.cpp` + the leak's third-party scan converter under `scaler/`, its own `xfonttt` library there) and the disk-file load path are both ported, so `XFONT_OpenTrueTypeFont` and `XFONT_OpenBitmapFont` exist. `font.h`/`xfont.c`/`bitmap.cpp` restored to the leak's full content. The scaler builds in its own slice on picolibc alone — it declares its own `ULONG`/`LPSTR`/`LARGE_INTEGER`, which redefine libxapi's; a small shim supplies the only two things it wants from the NT headers (`RtlZeroMemory`/`RtlCopyMemory`, and `ULONG_PTR` for `ClientIDType`). Note retail keeps XFONT **inside xgraphics.lib**, so the importer maps `xgraphics` → `libxgraphics;libxfont`. **Verified: DynamicFont + TrueTypeFont build to bootable .xbe.** Glyph rendering not HW-tested. |

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

## libd3d8 uplift plan (survey 2026-08-04 — ground truth verified)

Full symbol delta (leak `public/xdk/inc` vs 5849 headers, semantic diff ignoring reordering):
**131 deltas total** — d3d8types.h 47 added / 1 removed (`D3DTSS_TCI_SPHERE`) / 68 renumbered;
d3d8.h 13 added / 1 changed (`D3DVERTEXBUFFER_ALIGNMENT` 4→1); d3d8caps.h +`D3DPRESENT_INTERVAL_THREE_OR_IMMEDIATE`.

**1. Render-state renumbering (mechanical, verified safe).** Three shifts: simple section
grows +10 (`DEPTHCLIPCONTROL=82`, `STIPPLEENABLE=83`, `SIMPLE_UNUSED8..1=84..91`,
`SIMPLE_MAX` 82→92); deferred +9 more (`PRESENTATIONINTERVAL=127`, `DEFERRED_UNUSED8..1=128..135`,
`DEFERRED_MAX` 117→136); complex +1 more (`SAMPLEALPHA=158`, `MAX` 146→166). The runtime
(`se/state.cpp`) is fully symbolic with compile-time `VerifyHeaderFileEncodings()` asserts tying
every table size to the boundary constants — renumbering is a header change + table appends:
- `D3DSIMPLERENDERSTATEENCODE` (in BOTH shared/include/d3d8.h AND mirrored in se/state.cpp
  `g_RenderStateEncodings`): entries 0–81 verified numerically identical leak↔5849; append 10:
  `0x41d78` (=NV097 zmin/max control, DEPTHCLIPCONTROL), `0x4147c` (stipple enable), 8×`0x41d90`
  (the driver's parameterized-NOP method, same one the leak already uses for reserved slot 54).
- `D3DDIRTYFROMRENDERSTATE`: append 9 zeros (PRESENTATIONINTERVAL + 8 unused).
- `g_ComplexRenderStateFunctionTable` (state.cpp): insert `SetRenderState_SampleAlpha` at index
  158−136; `g_InitialRenderStates` (dxgcreate.cpp) gains matching initial values
  (DEPTHCLIPCONTROL=`D3DDCC_CULLPRIMITIVE`, STIPPLEENABLE=FALSE, rest 0).

**2. The 5849 `BeginState/EndState` direct-push contract.** 5849 titles inline push-buffer writes:
`D3D__Device[0]`=Put, `D3D__Device[4/4]`=Threshold, with exports `D3DDevice_MakeSpace` (slow path),
`D3DDevice_BeginStateBig(Count)`, `D3DDevice_EndState` (debug: `*ParameterCheck`). The leak already
guarantees the layout: `CDevice g_Device` (se/d3ddev.cpp) has `XMETAL_PushBuffer m_Pusher
{m_pPut,m_pThreshold}` as its FIRST member ("must be the first variable"), no vtable → the contract
is satisfied by exporting `g_Device` under the alias `D3D__Device` (+ `D3D__pDevice`) and thin
exports wrapping the existing `MakeSpace()`/pusher internals.

**3. New API surface (46 fns).** Classified:
- ~20 mechanical `*2` adapters (return-pointer variants of HRESULT+out-param originals:
  `CreateTexture2`, `GetBackBuffer2`, `GetRenderTarget2`, `Lock2`, `GetSurfaceLevel2`, …).
- 6 `SetVertexShaderConstant` variants (`1`, `4`, `Fast`, `1Fast`, `NotInline[Fast]`) — adapters.
- New NV2A features with real hardware paths: `Set/GetStipple` (stipple pattern methods),
  `Set/GetDepthClipPlanes` (+`D3DSDCP_*`/`D3DGDCP_*`), `SetRenderState_SampleAlpha`,
  `D3DRS_PRESENTATIONINTERVAL`.
- Pusher/misc: `MakeSpace`, `GetPushDistance`, `SetWaitCallback`, `SetTimerCallback`
  (+`D3DWAIT_*`/`D3DDISTANCE_*`), `GetViewportOffsetAndScale` (D3DVECTOR4),
  `SetRenderTargetFast`, `GetPersistedSurface2`, `PushBuffer_SetRenderState`,
  `PushBuffer_CopyRects`, `BeginState*`/`EndState*`.
- 5849 REMOVED `D3DDevice_Suspend/Resume` (keep ours; harmless superset).

**4. Header adoption.** Apply the delta to the RXDK-ported shared/include headers (hand-apply —
wholesale adoption would lose the RXDK portability transforms). The header-embedded tables and the
big `SetRenderState` inline dispatch must be lifted to 5849 content verbatim; lib-internal mirrors
must match (the debug verify catches drift).

**5. Rebuild order.** libd3d8 → every lib compiled against the d3d8 headers (libd3dx8, libxgraphics,
libdsound, libxact, libxonline/uix, libxvoice) → full sample sweep. `D3DVERTEXBUFFER_ALIGNMENT`
4→1: audit runtime alignment assumptions before relaxing.

Additional cross-cutting issue surfaced by the voice-sample sweep:

6. **MSVC `__inline` member definitions in a .cpp.** Several leak classes define members
   `__inline` in one TU while other TUs call them (e.g. libxact's `CEngine::AddRef` in
   engine.cpp, called from soundbank.cpp; the LowLevelVoiceChat sample's
   `CVoiceCommunicator::Submit*Packet`). MSVC emitted them anyway; standard C++ emits no
   out-of-line copy, so the cross-TU reference dies at link. Fix: drop the `__inline`.
   Related MSVC-ism: `friend` declarations don't inject the name at namespace scope
   (sample callbacks needed real forward declarations).

_Sample sweep: **177 of 181 XDKSamples projects build to .xbe** (`Platform=Xbox`). The denominator is 181, not 182: CreatePushBufferOnPC is a PC-side host tool with no Xbox configuration and should never have been imported as a title._

**Three sample sources — check all three before calling anything missing.** Most of what was
recorded as unrecoverable art was not. Besides our import there is the 5849 XDK's own
`Samples\Xbox` tree, and the leak's `private/atg/samples`, which is an EARLIER and in places more
complete vintage of the same samples. Between them they supplied `Resource.h` for DolphinHDTV /
FieldRender / PerPixelLightingVS (the XDK itself ships those three with no `.rdf` at all -- the
import was faithful), 14 assembled shader `.inl` files, the real per-sample `myfactory.h` for
DMMultiPass and DMTool, every missing `.wav`, and the `.bmp` textures. Assets can also sit under a
different name: Lensflare's `stonehengeground512.bmp` is Explosion's `stonehengeground.bmp`, verified
512x512x24 from the BMP header.

**Pixel shaders were tool output, not art.** The 5849 XDK ships `xbox\bin\xsasm.exe` (Xbox Shader
Assembler, Build 1.00.5849.1), and its default `.xpu` output turns out to be the tag `"PSB0"`
followed by a verbatim `D3DPIXELSHADERDEF` -- 60 DWORDs, so 244 bytes total. That was not assumed:
`tools/shaders/dump_psd.c` materialises a shipped `.inl`'s `PS_*` macro soup on the host so it can be
compared as numbers, and all 11 samples that shipped *both* halves (FocusBlur 6, Fur 5) come out
identical word for word. `tools/shaders/psh_to_inl.py` then generated the 7 missing files --
HighDynamicRange's `hotblur` / `extracthot` / `accumulate` and QuadLerp's `pshader0..3` -- and was
round-tripped against FocusBlur to pin field *order* as well as size. No `.inl` is missing anywhere in
the tree now. Caveat: `xsasm.exe` is a Win32 PE, so authoring new shaders off Windows still wants a
managed port (see `tools/shaders/README.md`); regenerating a checked-in `.inl` does not, and no build
step invokes the assembler.

**What is actually left:**
- **XACT WMA playlist -- API DONE, PLAYBACK NOT WIRED.** `IXACTWmaPlayList` is implemented from
  scratch in `libs/libxact/engine/wmaplaylist.cpp` (the leak has no WmaPlayList source anywhere), so
  XActWMAPlayList, TechCertGame and TechCertdemo now build. Building the song set, walking it
  ordered/shuffled/looping, removing entries, and reading the current song's title and duration out
  of its WMA header all work. What does NOT: a playlist is bound to a sound cue and playing that cue
  should stream the current song -- that path is still the ordinary wave-bank one, so a title can
  drive and display its playlist but will not hear it. Wiring it means teaching `CSoundCue` to
  render from an XMO and pumping it from `CEngine::DoWork`; `CWmaPlayList::OpenCurrentDecoder` is the
  hook it will use. All four add types work, including
  the user's own ripped soundtracks -- `XFindFirstSoundtrack`/`XGetSoundtrackSongInfo`/
  `XOpenSoundtrackSong` were already ported into libxapi (`k32/xsndtrk.c`), so the playlist opens a
  ripped song by handle instead of by path; `WmaCreateDecoderEx` takes either.
- **DSP Builder image, 2 samples.** FXMultiPass and GlobalFX need `DSPImage.h`, compiled from a
  binary `.fx` project whose `.scr` inputs are not in the sample either.
- **Port issues (2).** FastCPU (hand-tuned SSE asm restructure, deferred -- it is a benchmark, so a
  subtle error gives wrong numbers silently) and VolumeSprites (Bundler rejects VolumeTexture
  resources -- needs a 3D swizzle + resampler).

**PerfTest builds, but read what it reports carefully.** `D3DPERF_GetPushBufferInfo` is REAL -- size,
segment size/count, base/limit and bytes-written all come from the pusher. Every event counter and
the GPU busy/idle profile read exactly ZERO, because the XDK gathered those in a separate
instrumented build (d3d8i.lib) and RXDK ships the retail one. They are zero rather than
small-but-plausible precisely so the absence is visible; `GetStatistics` also says so once in the
debug log, and `StartPerfProfile` returns FALSE rather than handing back an all-zero profile that
would read as an idle GPU.

_Last updated: 2026-08-04. Tooling: `cvdump`, `dumpbin /DISASM` (RXDK-Tools). See also the samples/middleware memory note._
