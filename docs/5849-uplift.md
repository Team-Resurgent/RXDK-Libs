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
| libxact | `xacteng.lib` | ✅ 5849 | Full notification/marker ABI reconciled + the 5849 API surface for the sample suite. Reshape adapters: `PlayEx`, `PrepareEx`, `RegisterStreamedWaveBank`, `SetMasterVolume(+wCategory)`. Runtime `SetVariable`/`GetVariable` store, functional per-component listener setters; `SetParameterControl` remains the one no-op stub (RPC engine absent from the leak). Removed the leak's global `operator new/delete`. **The full 5849 API surface now exists: all ten formerly-missing `xact.h` APIs recovered from `xacteng.lib` and implemented** (`SetPitch`, `GetStatus`, `SetFilter`, `SetMode`, `GetProperties`, `EnableHeadphones`, `SetI3dl2Listener`, `GetRealtimeData`, `SelectVariation`, `GetSoundCueProperties`) — per-function recovered behaviour in `5849-xact-api-recovery.md`, incl. a live `g_lXactMemoryUsage` allocator counter for `GetRealtimeData`. **Verified: ALL 5 XACT samples build to bootable .xbe** (compile+link+imagebld). HW audio test pending. |
| libxonline | `xonline.lib` (+ `xonlinen`/`xonlines`, `uix.lib`) | ✅ 5849* | 5849 public `xonline.h` adopted; leak client compiles+links on libxnet(LIBO). **The UIX drop-in UI (`src/uix5849.cpp`) is fully functional** over the leak's real Live client: **logon** (multi-user 1–4 per-controller claims, guest sign-in, passcode entry, SILENT / RETRIEVED_STATE / RETRIEVED_GAME_INVITE starts, real `XOnlineLogon` task pumped in `DoWork`, connection held open after success), **friends** (real enumeration; accept/decline requests+invites, join, remove, invite-to-game, sign-out; `FRIENDS_JOIN_GAME[_CROSS_TITLE]` exits with the `XONLINE_FRIEND`), **players** (real `ILivePlayersList` registry + screen + mute menu + selection info), **notifications** (real, friends-state derived, property-gated), **`ILiveFriendsList`**, and a real `Reboot` (`XLaunchNewImage` to the dashboard). Also real: `XOnlineSilentLogon`, `XOnlineChangeLogonUsers` (re-logon with captured services), `XOnlineSave/RetrieveLogonState`, `XOnlineGetNotification` (friends-state derived), `XOnlineTitleIdIsSameTitle` (XBE cert). Rendering is the font-renderer contract by design (the retail skin pipeline is unbuildable on modern hosts). *Remaining asterisk: the 5849-new LSP services (stats v2/storage/messaging/arbitration/competition/teams/mutelist/offerings/query and peer QoS probes) report service-unavailable. **This IS open work, and the earlier note calling it a closed "protocol boundary" was wrong.** The wire formats are absent from the *leak*, but they are compiled into the shipped `xonline.lib`, and this project already recovers behaviour from retail libs by disassembly. The exported functions carry no protocol — they forward to internal workers (`XOnlineArbitrationReport` → `ArbitrationReportInternal` plus two zero arguments) — so the wire format lives in those workers and the task/HTTP layer below them. A stub makes a title build while hiding the gap, and cannot serve a community revival, which needs the client to speak the real protocol. Tracked as task #35. |
| libxvoice | `xvoice.lib` | ✅ 5849 | **Fresh implementation, NOT a leak port** — the leak has no xvoice source at all (only an older public header; codec + USB audio driver were binary-only). Adopted 5849 `xhv.h`/`xvoice.h`; XHV engine (`XHVEngineCreate` + 27 C exports + `_xhv_*_mode` globals) with REAL talker/mode/callback bookkeeping; communicator/codec paths faithfully report no-headset (`GetLocalTalkerStatus`=REMOVED, `IsTalking`=FALSE, packets accepted+discarded, VoiceMail/SR fail cleanly). Low-level `XVoiceCreate*` XMO factories fail as with no communicator (SC03 codec is binary-only in the retail lib). Exports `XDEVICE_TYPE_VOICE_{MICROPHONE,HEADPHONE}_TABLE` (zeroed → `XGetDevices` sees none). All 5849 stdcall decorations verified against the retail lib. |
| libdmusic | `dmusic.lib` | ✅ samples green | **All 7 DirectMusic samples build.** Fixes: the sample projects were never listing libdmusic at all; duplicate `GUID_NULL` and two `ReadMBSfromWCS`; `heap_rtl.h` missing its `extern "C"` (HeapAlloc resolved to a C++-mangled `RtlAllocateHeap`); and **`dsoundsequencer.cpp` now builds** in its own driver-env slice — it needs libdsound's bridge + internal includes, because dsoundi.h's kernel typedefs redefine the title-side ones, and the file is entirely `#ifdef XMIX`. Its two interfaces also had `Clear`/`ClearAtOrAfter` declared without `PURE` (the only such methods), so no vtable was ever emitted. Not HW-tested — no MIDI/DLS playback heard. |
| libd3d8 | `d3d8.lib` | ✅ 5849 | Full ABI reconciliation. Headers: 5849 `D3DRENDERSTATETYPE` adopted verbatim (three append-only shifts — simple +10, deferred +19, complex +20), all 47 new types/defines, the three header-embedded dispatch tables replaced wholesale, and the declaration / inline-dispatch / object-struct / wrapper regions lifted from 5849. Runtime: `g_RenderStateEncodings` +10 (new states use the same parameterized-NOP method the leak already used for its reserved slot), complex table + `SetRenderState_SampleAlpha`, `g_InitialRenderStates` extended at all three boundaries. **44 new exports** in `se/uplift5849.cpp` (the `*2` return-pointer adapters, 5 `SetVertexShaderConstant` variants, and real stipple / depth-clip-plane / SAMPLEALPHA / push-distance / wait+timer-callback implementations). Two ABI changes propagated into the lib: `BeginPush` returns the pointer, `CreatePixelShader` is void. The 5849 `BeginState`/`EndState` direct-push contract works via a linker alias of `D3D__Device` onto `g_Device` (whose pusher is already its first member by design). `D3DVERTEXBUFFER_ALIGNMENT` 4→1. **Verified: all 43 previously-green samples still build, zero regressions**; all dependent libs (d3dx8/xgraphics/dsound/xact/xonline/xvoice/xmv) rebuild clean. Not HW-tested. |
| libd3dx8 | `d3dx8.lib` | ❌ leak | |
| libxgraphics | `xgraphics.lib` | ❌ leak | |
| libdsound | `dsound.lib` | ✅ 5849 | Added `DSVOICEPROPS` (5849, recovered via cvdump) — needed by the 5849 xact.h. Added (header-only) `DSBCAPS_FXIN2`, `DSMIXBIN_VOICE_0..3`, `DSMIXBINVOLUMEPAIRS_{REQUIRED,DEFAULT}_5CHANNEL_3D[_PLUS_LFE]` for the voice/Marketplace samples. **Three runtime features implemented, each recovered by disassembling the retail lib rather than guessed:** (1) `DSBPLAY_SYNCHPLAYBACK` — `Play` arms the voice on but *paused* (register `0x140` is `NV1BA0_PIO_VOICE_PAUSE`, ACTION bit 18, `RESUME`=0), tags it `SYNCHPENDING` and bumps a counter; `IDirectSound_SynchPlayback` then resumes every pending voice inside one `VOICE_LOCK` at raised IRQL so they start on the same sample. (2) `DSBCAPS_MUTE3DATMAXDISTANCE` — distance volume becomes `DSBVOLUME_MIN` at/beyond `MaxDistance`; latched into `DS3DSOURCEPARAMS` at 3D-param allocation since it is creation-time. (3) **The WMA decoder XMO** — the leak has no WMA support at all. The ported ffmpeg WMA v1/v2 decoder moved here from libxact (matching 5849, where `dsound.lib` owns it and `xact.lib` calls in), joined by a new ASF reader (`wma/asf.c`: header-object walk + data-packet parsing incl. multiple/compressed payloads) and a packet-at-a-time front end (`wma/wmastream.c`), under `CWmaMediaObject` (`dsound/wmaxmo.cpp`) and the five 5849 factories. `XWmaDecoderCreateMediaObject` defers the header read to `DoWork` and withholds `ACCEPT_OUTPUT_DATA` until it lands — the handshake the streaming samples poll on. (4) `IDirectSoundBuffer_GetVoiceProperties` / `IDirectSoundStream_GetVoiceProperties` (5849-new; XACT `GetProperties` depends on them) — snapshot of the first hardware voice's **CUR** (ramped) volumes `NV_PAVS_VOICE_CUR_VOLA/B/C` unpacked `*100>>6` and negated (the exact inverse of `ConvertVolumeValues`), pitch from `TAR_PITCH_LINK` bits 31:16 sign-extended, pads `{0xFFFFFFFF, DSBVOLUME_MIN}`, unallocated voice → `DSERR_INVALIDCALL`; 3D/I3DL2 levels from `m_pHrtfSource->m_3dVoiceData` and `m_pI3dl2Source->m_I3dl2Data`, zeroed when not 3D. Retail's unpack takes volume 7's middle nibble from `VOLUME6_B7_4` — a bug contradicted by its own setter, not replicated (see `5849-xact-api-recovery.md`). Header ABI: 5849's `DoWork` on `XFileMediaObject`/`XWaveFileMediaObject` and `DoWork`+`SeekToTime` on `XWmaFileMediaObject`, adopted in both `dsound.h` and `dsoundp.h`. **Verified: MultipleListeners, SetRolloffCurve, WMAStream, WMAInMemory, EnumSoundtrack, BackgroundMusic and FastLoad all build to bootable .xbe.** Not HW-tested. |
| libxapi | `xapilib.lib` | 🔶 partial | Added the 5849 `XSaveFloatingPointStateForDpc`/`XRestoreFloatingPointStateForDpc` (k32/fpudpc.c — single static `KFLOATING_SAVE`; DPCs are serialized on the single-CPU Xbox). Rest unassessed. |
| libxnet | `xnet.lib` / `xnets.lib` (+ `xneto`) | ✅ split | `INVALID_SOCKET`/`XNetGetConnectStatus` glue for the Live samples lives here. **Now built in the two variants the stack's own `xnp.h` defines, because they are not nested the way "superset" suggests:** `libxnet.lib` is `XNET_BUILD_LIBX` (ARP/DHCP/DNS/FRAG/ICMP/INSECURE/ROUTE — plain sockets) and `libxneto.lib` is `XNET_BUILD_LIBO` (adds ONLINE/QOS/SG). The build was LIBO-only on the reasoning that a superset harms nobody; it does, because `XNET_FEATURE_SG` compiles the secure-gateway Kerberos exchange into `ip.cpp`, which calls `CXoBase::XoKerb{GetInfo,BuildApReq,CrackApRep,GetAuthData}` — four symbols libxonline defines. `CXoBase` is deliberately one class split across two libs (`xn.h` labels the halves "XNet Support for XOnline" / "XOnline Support for XNet"), so a LIBO libxnet cannot link without libxonline even for a title that only wants a UDP socket. `samples/xnet-net` had been failing to link for exactly this reason. **Verified: all 36 XNet samples build; WinsockPeer and HostMigration build as pure sockets titles on libxnet with libxonline removed entirely** — which is how their original XDK `.vcproj` link `xnets.lib`. Not HW-tested. |
| libxmv | `xmv.lib` | ✅ exports | **Full retail export parity.** The port implemented the `GetNextFrame` path (title drives decoding and presents each frame); the remaining 13 exports are now in: `Play` (blocking whole-movie playback through the D3D overlay, so it can run on a background thread without disturbing the title's own rendering), `CreateDecoderForPackets` (drains the title's callbacks into one image up front — the demuxer works over a complete in-memory image, as `CreateDecoderForFile` already did), `Reset`, `Terminate{Loop,Playback,Immediately}`, `Get/SetSynchronizationStream`, `GetAudioStream`, `DisableAudioStream`, `GetTimeFromStart`, `g_XMVInhibitDebugOutput`, `XMVBuildNumber`. Header quirks worth knowing: `Play`'s prose names `XMVPLAY_LOOP`, which xmv.h never defines (`XMVFLAG_FULL_LOOP` is the real bit); `GetAudioStream` returns a **borrowed** pointer with no AddRef, matching `EnableAudioStream`. **Verified: SimpleXMV, XMVCompare, XMVPlayer all build to bootable .xbe.** Video decode is **implemented, not a placeholder**: keyframes go through the leak I-frame kernel or the ported IntraX8 path, P-frames through the ported WMV2 layer (picture header, macroblock loop, motion compensation, residual). Geometry is not a fallback either — `XmvCoreCreate` aligns coded planes up to macroblock multiples and the render crops. `RenderPlaceholder` survives only for frames before the first keyframe, or an unallocatable core. What remains is verification against a reference decoder / hardware, not implementation. |
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

_Sample sweep: **ALL 181 of 181 XDKSamples projects build to .xbe** (`Platform=Xbox`). The denominator is 181, not 182: CreatePushBufferOnPC is a PC-side host tool with no Xbox configuration and should never have been imported as a title._

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

**libxact runtime notes** (the matrix row says libxact is reconciled to 5849, which is true of the
ABI; this is the runtime detail behind it):
- **Pause is implemented.** `GlobalPause` used to be an empty stub -- it returned S_OK having paused
  nothing. It now walks the active cues and the WMA playlists and pauses each voice in place.
  `IXACTSoundBank_PauseSoundCue` landed with it. The wrinkle worth knowing: a DirectSound *stream*
  has `Pause`, a *buffer* does not, so a buffer voice is paused by stopping it and remembering the
  play cursor -- plain Stop/Play would retrigger the sound from the start.
- **`wCategory` selects on `GlobalPause`.** `XACT_SOUNDBANK_SOUND_ENTRY` carries `wCategory`, so the
  sound entry is 30 bytes and the bank version is **2**. Stale `.xsb` files error cleanly (the engine
  validates the version strictly) -- rebuild them. xactbld writes the index in the same order it
  emits the `XACT_CATEGORY_*` enumerators, which is required, since a title passes one of those
  straight to `GlobalPause`.
- **`SetMasterVolume` applies, per category.** It was ALSO an empty stub (it validated its argument
  and did nothing, so a title's volume slider moved nothing). Now the volume is stored per category
  -- `XACT_SOUNDBANK_CATEGORY_UNUSED` addressing the global master -- and the two compose. Volumes are
  hundredths of a dB, so composing is an add. Unlike pause, a volume is persistent state, so it is
  applied in two places: the sequencer composes it whenever content sets a volume (so later sounds
  pick it up), and `SetMasterVolume` re-applies to cues already playing. That re-application is why
  `CSoundSource` remembers the BASE volume -- neither a buffer nor a stream has `GetVolume`, so
  without it repeated changes would compound and the mix would drift down.
- **RPC parameter modulation** (`SetParameterControl`) is a no-op; variables are stored but do not
  drive sound parameters.

**What is actually left:**
- **XACT WMA playlist -- DONE, including playback.** `IXACTWmaPlayList` is implemented from scratch
  in `libs/libxact/engine/wmaplaylist.cpp` (the leak has no WmaPlayList source anywhere). All four add
  types work, user-ripped soundtracks included -- the enumeration APIs were already ported in
  `libxapi/k32/xsndtrk.c`, so a ripped song opens by handle where a file song opens by path, and
  `WmaCreateDecoderEx` takes either. Playback is wired: a playlist owns a DirectSound stream and
  renders itself rather than going through a cue's wave-bank path, `CSoundBank::Play`/`Stop` divert to
  it for a playlist-bound cue, and `CEngine::DoWork` pumps it. The ring waits for queued packets to
  drain before advancing the song (advancing at decoder-EOF would clip every track's tail), and packet
  size is rounded to a whole sample frame because `XMO_STREAMF_FIXED_SAMPLE_SIZE` makes `dwOutputSize`
  one frame rather than a buffer size. NOT hardware-tested -- no audio has been heard from it.
- **DSP Builder image, 2 samples.** FXMultiPass and GlobalFX need `DSPImage.h`, compiled from a
  binary `.fx` project whose `.scr` inputs are not in the sample either.
**There is no art gap.** The last two, FXMultiPass and GlobalFX, needed `DSPImage.h` -- which is
tool output, not art. `dspimage.fx` ships with both samples and the XDK ships `dspbuilder.exe`; it is
a GUI, but the original `.vcproj` documents a batch rule (`dspbuilder /B <fx> /Ob <bin> /Oh <h> /Oi
<inidir>`) and `dspbuilder.com` pipes its stdio. It only failed with "Unable to load effects" because
the XDK here is unpacked rather than installed -- point `_XGPIMAGE_DSP_CODE_PATH` at
`<XDK>\Source\DSound\dspin` and `_XGPIMAGE_INI_PATH` at `...\ini` and it builds headlessly. The
generated `.h`/`.bin` are checked in, so building stays cross-platform; only authoring a new DSP graph
needs the tool.

Every earlier port issue is closed too. FastCPU was verified by DISASSEMBLING the object and counting
the loop body against the source (movss 8, shufps 24, movaps 36, mulps 32, addps 27, movlps 2, mov 9,
add 13, sub 1, cmp 9, je 7, jne 2) -- all exact -- because it is a benchmark, where a subtle error
gives wrong numbers rather than a failure. Still not run on hardware.
**VolumeSprites builds**: the Bundler now handles VolumeTexture resources. `Swizzle.SwizzleBox3D` is
a port of the `Swizzler` class in the public `xgraphics.h` (the one `XGSwizzleBox` drives), NOT an
extension of `GetMasks2` -- the latter splits U/V by a lower/upper mask, while `Swizzler` interleaves
one bit per axis at a time, which is what makes a non-cubic volume like 16x16x4 come out right. Mips
halve all three axes and average eight texels across two slices. NOTE: no golden `.xpr` exists from
the retail bundler for a volume, so this is verified by construction and by the sample building, not
byte-compared like the rest of the port. **Needs a tools release before it reaches users.**

**PerfTest links `libd3d8i`, the instrumented D3D, and its counters are real.** The XDK shipped two
builds of the driver and so do we: the instrumentation (counters, push-buffer accounting, GPU
profile, and all fifteen D3DPERF entry points in `se/stats.cpp`) sits behind `#if PROFILE`, so
`libd3d8i` is the same sources with `-DPROFILE=1`, not a fork. The split matches retail exactly --
5849's `d3d8.lib` carries only `D3DPERF_BeginEvent`/`EndEvent`/`SetMarker` while `d3d8i.lib` and
`d3d8d.lib` add `GetStatistics`/`Reset`/`GetPushBufferInfo`/`Start`+`StopPerfProfile`. A title that
profiles selects `libd3d8i` via `RxdkLibraries`; a shipping title keeps retail and pays nothing for
counters it never reads. The variant is opt-in (not in the default install, since it doubles the d3d8
compile) but is shipped in `dist`.

_Last updated: 2026-08-04. Tooling: `cvdump`, `dumpbin /DISASM` (RXDK-Tools). See also the samples/middleware memory note._

## Library ownership: is each symbol in the lib 5849 puts it in?

A symbol can be implemented correctly and still sit in the wrong archive. Nothing
in a sample build catches it -- the link line names every lib, so it resolves
either way -- and it only bites a title that links a subset. That is precisely
what the libxnet LIBX/LIBO split turned out to be.

`tools/lib_ownership_audit.py` compares the defined-symbol set of every one of our
libs against the retail 5849 libs and reports the differences. Current state:
**4403 symbols in common, 161 placed differently.** Ignoring symbols that appear
in nearly every 5849 lib (compiler/CRT helpers, not owned code), the real buckets
are:

| Count | We put it in | 5849 puts it in | Reading |
|---|---|---|---|
| ~~53~~ 0 | `libxact` | `xacteng` | **Not a real difference.** 5849 ships NO `xact.lib` — `xacteng.lib` IS the XACT library, and 58 of our libxact symbols are in it. The audit's alias table mapped `libxact` to a library that does not exist. Only the archive NAME differs. |
| ~~49~~ 0 | `libxonline` | `uix` | Real, and **now split**: 5849 ships `uix.lib` separately and we had folded it in. See below. |
| 16 | `libxnet` | `xonline` | `CXoBase::Xn*` and `CXnIp::Ip*` — **the opposite of what `xn.h`'s own comment implies.** The header labels the `Xn*` half "XNet Support for XOnline", but 5849 compiles the whole class into `xonline.lib`. Directly relevant to the LIBX/LIBO split. |
| 1 | `libxnet` **and** `libxonline` | `xonline` | `XOnlineBuildNumber` — a **genuine duplicate**, now fixed (see below). |
| ~~7~~ 0 | `libxapi` | `libc` | `_snprintf`, `_stricmp`, `_scprintf`, `_vsnprintf` and friends — **moved to libc**, see below. |
| 3 | `libxapi` | `xkbd`/`xkbdd` | `XInputDebugInitKeyboardQueue`, `XInputDebugGetKeystroke`, `XDEVICE_TYPE_DEBUG_KEYBOARD_TABLE`. Retail makes keyboards an opt-in dev-time library; **RXDK keeps them in libxapi deliberately** (user decision 2026-08-06): keyboard — and eventually mouse — are to be first-class, non-debug input for titles. Do not split this out for retail parity; work here goes the other way, toward ordinary (non-`Debug`-gated) input paths. |

None of these are wrong *code*; they are packaging differences, and each is a
separate decision about whether to match 5849's archive layout or keep ours.
Recorded here rather than fixed silently, because splitting a shipped lib changes
what a title must name on its link line.

### A false positive worth recording

The audit's first run also reported 55 `Ke*`/`Ex*`/`Io*`/`Hal*` symbols as defined
by both `libxapi` and `libkernel`. They are not. `libkernel` supplies the
**stdcall** xboxkrnl imports (`_KeGetCurrentIrql@0`) and `libxapi` supplies
**undecorated** entry points (`_KeGetCurrentIrql`) — different symbols with
different calling conventions. The audit had been stripping the `@n` suffix to
match MSVC names against ours, which invented 55 duplicates that never existed.
It no longer strips it, which is why the totals moved from 216 to 161.

### The one real duplicate, fixed

`XOnlineBuildNumber` was emitted by both libraries: `libxonline` via `VERXON`, and
libxnet's LIBO build via `VERXNET`, which the leak names `XOnlineBuildNumber`
outright. A title linking both got two definitions of one symbol, resolved to
whichever archive member the linker pulled first.

Retail 5849 does not do this, and the shipped libs say so plainly: every
`xnet*.lib` stamps `XNetBuildNumber*`, only `xonline*.lib` stamps
`XOnlineBuildNumber*`. The online xnet build now stamps `XNetBuildNumberO`,
leaving `XOnlineBuildNumber` to libxonline alone.

Reading those libs also settled what the `xnet` variants actually are, which
matters for the LIBX/LIBO split: 5849 ships `xnet`/`xnetn`/`xnets` (plus `d`
debug forms) and **no `xneto.lib` at all**, and the suffixes pair across
libraries — `xnets` with `xonlines`, `xnetn` with `xonlinen`. So 5849's axis is a
build flavour spanning both libs, not the sockets-versus-online split the leak's
`xnp.h` describes. Our split solves a real linking problem (see the libxnet row
above) but is not the shape 5849 shipped.

### The CRT functions moved to libc

`_stricmp`, `_wcsicmp`, `_snprintf`, `_snwprintf`, `_scprintf`, `_vscprintf` and
`_vsnprintf` were defined in `libxapi/port/compat.c`. 5849 ships them in
`libc.lib`/`libcmt.lib`, and they are CRT by nature — picolibc simply does not
provide MSVC's underscore spellings. They now live in `libs/libc/xbox/msvc_crt.c`.

Split out rather than moved wholesale: `port/compat.c` is otherwise `NtCurrentTeb`
and xapi startup, which belong exactly where they are.

Worth preserving with them is the reason `_vsnprintf` is not a one-line forward to
`vsnprintf`. MSVC's count argument does not mean C99's: it writes at most `count`
characters, terminates only if the result fits in fewer, and returns -1 on
truncation. Xbox-era code is written to that rule, so the measure-then-fill idiom
loses its last character if the size is treated as C99's — which surfaced once as
a title rendering wrong glyphs, because its lookup keys were built that way and
"65" became "6".

### uix.lib split out of libxonline

Of our libxonline's symbols, 813 are in 5849's `xonline.lib` — the right place —
and 50 are in `uix.lib`, which 5849 ships as a separate archive. All 50 came from
one translation unit, `src/uix5849.cpp`, so the split was a clean cut: same
compile environment, its own archive.

`libuix.lib` is 216 KB against libxonline's 4.6 MB. It calls into libxonline, so a
title links both, libuix first. The importer maps `uix.lib` to `libuix;libxonline`
accordingly, and the seven samples that used UIX (the five `UIX*` ones plus
SimpleVoice and SingleElimination) now name it.

### Two of the three original rows were the audit's fault, not the build's

Worth stating plainly, because the audit is meant to be trusted:

- The 55 `libxapi`/`xboxkrnl` "duplicates" came from stripping the `@n` stdcall
  suffix, which conflated `_KeGetCurrentIrql` with `_KeGetCurrentIrql@0`.
- The 53 `libxact`/`xacteng` "misplacements" came from an alias table that
  expected a library named `xact.lib`, which 5849 does not ship.

Both are fixed in `tools/lib_ownership_audit.py`. The lesson is the same one the
rest of this document keeps recording: check the premise against the shipped
artifact before acting on it. Two of these three rows would have been work done
to fix nothing.

## Missing public constants — `tools/const_sweep.py`

The quietest of the three "up to 5849" measures: not "does this function exist"
(the api audit) or "is it in the right lib" (the ownership audit), but "does this
named value exist". A 5849 `#define` we don't carry fails a title's *compile*, so
unlike a missing API it is caught the moment a title's source names it — but it
is still real ABI, and it flushes out headers we adopted only partially.

**Real total: 0.** Every Xbox-SDK-header constant 5849 defines, we now define.
The last gaps closed were **DSound.h (8** — the 5849 stream additions:
`DSSTREAMCAPS_MUTE3DATMAXDISTANCE`/`_NOCOALESCE`, `DSSTREAMPAUSE_SYNCHPLAYBACK`/
`_PAUSENOACTIVATE`, `DSSTREAMFLUSHEX_ENVELOPE2`, `DSSTREAMSTATUS_ENVELOPECOMPLETE`,
`DSSTREAMVOLUME_HW_MIN`, `DSI3DL2_ENVIRONMENT_PRESET_DEFAULT2`**)** and **XbDm.h
(39** — the debug-monitor allocation-type table `DM_ALLOCTYPE_*`, the upper
`DM_TRACK_*` bits, stack-trace/bugcheck notify types, and the D3D/VX/profile
`XBDM_*` result codes; adding `DM_STACKTRACE`/`DM_BUGCHECK` also bumped
`DM_NOTIFYMAX` 18→20 to match**)**. Values taken verbatim from the 5849 headers,
placed beside their siblings; libxbdm rebuilds and all 181 samples still compile.

⚠️ **What the sweep does NOT count, by design** (both are ours-by-choice, not
gaps): the picolibc CRT headers (`tchar.h`, `crtdbg.h`, …, ~274 constants — our
libc is the deliberate replacement) and the inherited Win32 Platform-SDK headers
(`WinNT.h` alone is ~900: ACL/token access masks, PE image directory entries,
`EMARCH_ENC_*` Itanium encodings, `ES_*` execution-state — the Xbox kernel
implements none of it, so our `WinNT.h` is an intentional subset). Folding those
into the total buries the real Xbox-SDK signal — the first run reported 953 and
was useless until they were separated out.

⚠️ **The tool's own first-run bug, kept as a cautionary note in its header:** a
whole-file scan for the include-guard `#ifndef X\n#define X` shape wrongly matched
the overridable-default idiom `#ifndef STRICT / #define STRICT 1` (a real valued
constant, mid-file), silently dropping `STRICT` and reporting a false gap.
Fixed by anchoring guard detection to the *first* directive in the file. Every
measurement tool in this suite has been wrong on its first run; the first number
always deserves the same suspicion as the code.

### Defined, but with the WRONG value

A second pass (`reduce_int`) compares the *values* of constants defined on both
sides — a constant we define wrongly is worse than one we omit, because it
compiles and misbehaves. It only compares what reduces to an integer without
expanding any identifier or macro call, so it reports real numeric disagreements
and stays silent where it cannot be certain. It found **12**, every one our port
faithfully copying a *leak* value that 5849 later changed. **Ten adopted to 5849,
two kept as intentional divergences:**

- `DSBVOLUME_HW_MIN` −10000 → **−6400**: we had conflated the hardware floor with
  the software `DSBVOLUME_MIN` (−10000); 5849 documents −6400. Not library-used;
  the fix also corrects the `DSSTREAMVOLUME_HW_MIN` that aliases it.
- `DSEG_{ATTACK,DECAY,DELAY,HOLD,RELEASE}_MAX` 8191 → **4095** (public + private
  headers): libdsound validates envelope descriptors against these by name, but
  that validation is debug-only (`DPF_ERROR` is a release no-op) and 4095 ⊂ 8191,
  so adopting the 5849 clamp is purely safe.
- `XNET_QOS_LISTEN_{ENABLE,DISABLE,SET_DATA,SET_BITSPERSEC}` 0/1/2/4 → **1/2/4/8**:
  a real bug fixed. libxnet tests these as bit flags (`dwFlags & …`), so the
  leak's `ENABLE = 0x00` was a dead test that never fired, and the leak numbering
  put a 5849 title's `SET_BITSPERSEC` (0x08) outside the validation mask, getting
  it rejected. libxnet uses the names, so the renumber propagates cleanly.
- `WAVEBANKHEADER_VERSION` (2 vs 1) and `WAVEBANKHEADER_BANKNAME_LENGTH` (16 vs
  64) — **kept.** Our on-disk wave-bank format is the leak *trunk's* version 2;
  xactbld emits it, `wavebank.cpp` rejects any other version, and the header's
  name-field length is part of that struct. It is a closed tool↔engine contract,
  not a title-facing ABI (titles never parse `.xwb` at runtime), so matching 5849
  would reformat our banks for no caller's benefit. Recorded in the tool's
  `KNOWN_VALUE_DIVERGENCE`, so the pass stays at zero without hiding the fact.

The rule the pass encodes: when our value is faithful to the leak and 5849
changed it, 5849 wins — that *is* the uplift — but only after confirming the
library reads the constant by name (so a header edit plus a rebuild stays
consistent) and nothing in the build regresses.

## Missing public APIs, not just missing constants

`tools/api_gap_audit.py` asks the harder question: not "does this symbol have a
value" but "does this function exist at all". A title calling a missing API fails
to *link* — louder than a missing constant, but easier to miss in development,
because nothing in our sample suite calls any of them.

A name counts only if it is BOTH declared in a 5849 public header with an
exported-function shape AND defined in a shipped 5849 library. That intersection
is what makes the number meaningful: the headers alone carry macros and inline
helpers that were never exported, and the libraries alone carry a great deal of
internal code that was never public — 5849's `xvoice.lib` contains the
binary-only codec, which is not a gap in our port so much as something we were
never going to have.

**6 public APIs absent from our libs** (was 64). Closed this arc: the ten `xact.h` gaps (see
`5849-xact-api-recovery.md`) plus `IDirectSound{Buffer,Stream}_GetVoiceProperties` that
`IXACTSoundSource_GetProperties` needed (hidden by the audit's dsound.lib-ends-in-'d' bug);
`XAudioSetEffectData` and `D3DPERF_QueryRepeatFrame` (DSound.h + D3D8Perf.h rows); four of nine
`Xbox.h` gaps (`5849-xbox-h-recovery.md`); and all 43 `xonline.h` gaps (40 service-unavailable
stubs completing the existing pattern + 3 done for real — `TitleIdIsSamePublisher`, `Throttle`
Get/Set). The remaining 6 are the two heavyweight `Xbox.h` write subsystems (5) and
`XGCompileShader` (1, blocked on the xsasm vertex back end):

| Header | Missing | What they are |
|---|---|---|
| ~~`xonline.h`~~ | ~~43~~ 0 | All 43 now defined. **40 stubbed to complete the existing service-unavailable pattern** in `src/uplift5849.cpp` — a title calling one now links and gets a clean failure instead of an unresolved symbol; the wire protocols remain absent from the leak (see below). **3 done for real / retail-exact**, not stubbed: `XOnlineTitleIdIsSamePublisher` (pure computation — compares the publisher word, high 16 bits, of the session's own XBE-certificate title ID against the argument), and `XOnlineThrottleGet`/`XOnlineThrottleSet` (the shipped 5849 lib's *own* bodies are `E_NOTIMPL` stubs — the client throttle governor was never built — so `E_NOTIMPL` is the faithful match). |
| `Xbox.h` | ~~9~~ 5 | **4 recovered and implemented** from retail xapilib: `XGetAutoLogonFlag` (misc-flags EEPROM bit 0x4 → the `XC_AUTO_LOGON_ALLOWED`/`NOT_ALLOWED` codes), `XSet`/`XGetAttributesOnHeapAlloc` (the `HEAP_ENTRY` reserved dword just before the user pointer), `XGetFilePhysicalSortKey` (FATX starting cluster / XDVDFS starting sector, keyed off the volume's FS-name dword). **Left (5), both heavyweight write-side subsystems the read/abstracted port never carried, and with no sample or known-title user — scoped follow-ups, not stubs:** soundtrack *write* (`XAddSoundtrack`, `XAddSongToSoundtrack` — needs the full ST.DB block manager + MUSIC-dir WMA copy; we have only the enumeration side) and the secondary utility drive (`XMountSecondaryUtilityDrive`, `XSwapUtilityDrives`, `XFormatSecondaryUtilityDrive` — built on retail's raw refurb-sector cache-partition database and `g_iZDriveDBIndex`/`g_iNDriveDBIndex`, which our `XapiSelectCachePartition` deliberately replaced). See `5849-xbox-h-recovery.md`. |
| ~~`DSound.h`~~ | ~~1~~ 0 | `XAudioSetEffectData` — **recovered and implemented**: converts high-level IIR2/distortion/I3DL2-reverb parameters to raw DSP state. The biquad math is RBJ peaking-EQ at 48kHz (`A=10^(dB/40)`, `α=sin(ω)·sinh(1/2Q)`) in 1.23 saturating fixed point; the I3DL2 path fetches the running image's `State+DelayLines` (0x118 bytes), runs the leak's own `CI3dl2Listener::CalculateI3dl2` over it, and pushes back the flags word and the 0x108-byte tuning, deferred + commit. ⚠️ 5849's public `DSFX_RAW_EFFECT_DESCRIPTION.Distortion` view is **mislabeled by one dword** (the function writes gain first, then ten coefficients — eleven dwords into a ten-field struct); adopted verbatim with the quirk documented. |
| ~~`D3D8Perf.h`~~ | ~~1~~ 0 | `D3DPERF_QueryRepeatFrame` — **implemented in both flavors**. Retail's plain d3d8.lib body is an unconditional FALSE (`xor eax,eax; ret`); only d3d8i.lib consults the capture tooling's repeat-frame request, for which this stack has no writer, so FALSE is faithful for both. Kept in shadersnapshot.cpp, retail's own TU for it. |
| `XGraphics.h` | 1 | `XGCompileShader` — the HLSL compiler. See below. |

**On the XOnline 43 — now closed at the link level.** *Absent* is worse than
*failing*: a title calling one used to fail to link, where 5849 would link and
return a service error. The 40 wire-protocol entries now carry clean
service-unavailable stubs (the same behavior the rest of our LSP surface already
had), so such a title builds and degrades gracefully. This is NOT the same as
implementing the services: the wire formats are still absent from the leak, and
a community revival still needs the client to speak the real protocol (recover
the internal workers — `XOnlineArbitrationReport` → `ArbitrationReportInternal`
plus two zero args — and the task/HTTP layer beneath them, not these exported
wrappers). What changed is only that the API surface is complete and linkable.
Three of the 43 were never wire calls and are done for real: the disassembly
showed `XOnlineTitleIdIsSamePublisher` is a publisher-word compare, and the
shipped lib's own `XOnlineThrottleGet`/`Set` bodies are `E_NOTIMPL`.

**On `XGCompileShader`:** it is 5849's HLSL front end, and its own header settles
a question worth recording — targets are `vs.1.1`/`xvs.1.1`/`xvss.1.1`, and
"Pixel shader compilation (ps.1.1 and xps.1.1) is not currently supported".
Microsoft did not support HLSL for pixel shaders either, for the same reason the
managed port found: `D3DPIXELSHADERDEF` configures register combiners rather than
holding a program, so there is no general lowering to target. The leak carries no
HLSL compiler source, so this would be a from-scratch front end, and it sits *on
top* of the assembler — it compiles HLSL to `vs.1.1` and then assembles it, so it
is blocked on the xsasm vertex back end.