# Recovering the missing XACT APIs from `xacteng.lib`

Ten public `xact.h` APIs are declared in our header but implemented nowhere, so a
title calling them fails to link (see the API-gap section of `5849-uplift.md`).
They are absent from the January-2002 leak — but they are *present* in the 5849
retail library, so the behaviour is recoverable rather than guessable.

This file records what has been read out of the binary so far, so the
implementation is written against fact.

## Method

The API entry points live in one archive member of `xacteng.lib` (offset
2560540); the `CSoundSource` bodies in another (1084522) and `CEngine` in a third
(1714956). Extract a member in Python — 60-byte header, decimal ASCII size at
offset 48 — then disassemble it:

```
dumpbin /DISASM:BYTES member_1084522.obj
```

Run `dumpbin` from PowerShell. git-bash mangles `/SWITCH` arguments into paths.

## What the entry points are

Thin forwarders, matching the shape `engine/xactapi.cpp` already uses:

```
IXACTSoundSource_SetPitch    -> jmp ?SetPitch@CSoundSource@XACT@@QAGJJ@Z
IXACTSoundSource_GetStatus   -> jmp ?GetStatus@CSoundSource@XACT@@QAGJPAK@Z
IXACTEngine_EnableHeadphones -> (pEngine - 8), null preserved, then
                                ?EnableHeadphones@CEngine@XACT@@QAGJH@Z
```

The mangled names give exact signatures: `QAG` is public `__stdcall`, `J` is
LONG, `PAK` is `DWORD*`, `K` is DWORD, `H` is int/BOOL.

⚠️ **`IXACTEngine_EnableHeadphones` adjusts `this` by −8 before calling**, and
preserves a null pointer while doing it (`neg`/`sbb`/`and` — the branchless
"if p then p-8 else 0"). No header and no source states that offset; it exists
only in the binary, and an implementation that ignores it is quietly wrong.

## Recovered behaviour

### `CSoundSource::SetPitch(LONG)`

Takes the object's local lock (`this+0x24` is the CRITICAL_SECTION), then
dispatches on which voice kind is present:

```
if (this+0x1C /* pBuffer */)  return IDirectSoundBuffer_SetPitch(pBuffer, lPitch);
else                          return IDirectSoundStream_SetPitch(pStream /*this+0x20*/, lPitch);
```

Buffer is tested **first**, stream is the fallback — the same precedence our
`Pause` uses, and worth keeping identical.

### `CSoundSource::GetStatus(DWORD*)`

```
lock;
*pdwStatus = 0;
if (this+0x08 > 1 || IsPlaying())   *pdwStatus |= 1;
```

So bit 0 is "playing", and the state word at `this+0x08` short-circuits the
`IsPlaying()` call when it is greater than 1 (unsigned) — the comparison is `ja`,
not `jg`.

### `CSoundSource::SetMode(DWORD dwMode, DWORD dwApply)`

The deferred-settings pattern rather than a direct call:

```
lock;
this+0x44  |= 0x400000;      // dirty flag for this setting
this+0x84   = dwMode;
for (each entry in the list at this+0x15C)
    entry[-0x110] |= 0x400000;
    entry[-0x0D0]  = dwMode;
```

i.e. it marks the setting dirty and stores it, on the source *and* on every
attached voice, leaving `CommitDeferredSettings` to push it. That matches how the
other `dwApply`-taking setters behave and is why `SetMode` cannot simply forward
to DirectSound.

### `CSoundSource::SetFilter(const DSFILTERDESC*)`

Structurally identical to `SetPitch`: same lock, same buffer-then-stream
dispatch, forwarding the descriptor pointer unchanged.

### `CSoundSource::GetProperties(XACT_SOUNDSOURCE_PROPERTIES*)` — fully read

```
lock;
pProps->dwHighestCuePriority = (DWORD)(WORD)this+0x12;
hr = IDirectSoundBuffer_GetVoiceProperties(this+0x1C /* pBuffer */,
                                           &pProps->HwVoiceProperties);
if (SUCCEEDED(hr) && (this+0x14 & 2))   // 3D
{
    pProps->HwVoiceProperties.l3DDistanceVolume  = this+0x0EC;
    pProps->HwVoiceProperties.l3DConeVolume      = this+0x0F0;
    pProps->HwVoiceProperties.l3DDopplerPitch    = this+0x10C;
    pProps->HwVoiceProperties.lI3DL2DirectVolume = this+0x104;
    pProps->HwVoiceProperties.lI3DL2RoomVolume   = this+0x108;
}
return hr;
```

The five trailing writes looked at first like fields past the end of the struct,
which would have meant our header was missing members. It is not: `DSVOICEPROPS`
is 92 bytes, so `HwVoiceProperties` spans 0x04–0x5F and those writes land on its
**last five fields**. Our header matches 5849's exactly — two members.

So for a 3D voice the function *overwrites* the five 3D/I3DL2 volumes that
`GetVoiceProperties` just filled, using values XACT caches on the source itself.

⚠️ The source offsets are **not** in field order: `l3DDopplerPitch` takes
`this+0x10C` while the two I3DL2 volumes take `0x104` and `0x108`. Reading them
as sequential would put the doppler pitch and the direct volume in each other's
slots — a plausible-looking result that no build would catch.

Still blocked on our side, not retail's: those five values live at fixed offsets
in retail's `CSoundSource`, and our class is a port with a different layout. The
question is which of our members hold the cached 3D distance/cone/doppler and
I3DL2 direct/room volumes — or whether the port caches them at all, in which case
they have to come from somewhere else.

**Update:** implemented. `IDirectSoundBuffer_GetVoiceProperties` and
`IDirectSoundStream_GetVoiceProperties` are recovered and implemented in
libdsound (see `5849-uplift.md` and `CMcpxVoiceClient::GetVoiceProperties` in
`libs/libdsound/dsound/mcpvoice.cpp`), and `IXACTSoundSource_GetProperties`
forwards to them from `CSoundSource::GetProperties`.

The `CSoundSource` cache question above resolved itself once the dsound side
was read: retail XACT's five-value overwrite re-applies *cached copies* of the
same computed 3D values that our libdsound `GetVoiceProperties` reads live from
`m_pHrtfSource->m_3dVoiceData` / `m_pI3dl2Source->m_I3dl2Data`. The overwrite
exists in retail because retail-dsound and retail-XACT kept separate copies; in
this port there is one copy, so there is nothing to overwrite. The
`dwHighestCuePriority` word (retail `this+0x12`) is tracked honestly from the
soundbank's per-sound `wPriority` when a cue's Play event binds a voice
(`NoteCuePriority`), reset when the voice stops; the retail engine code that
maintains its copy was not read, so this is equivalence, not disassembly.

### The dsound side, for the record

Retail's `CMcpxVoiceClient::GetVoiceProperties` reads the **CUR** (ramped)
hardware volumes of the first hardware voice — `NV_PAVS_VOICE_CUR_VOLA/B/C` at
`+0x28/+0x2C/+0x30` of the 128-byte voice struct, not the TAR targets — plus the
pitch word from `TAR_PITCH_LINK` (`+0x7C`, upper 16 bits, sign-extended). Each
12-bit attenuation is scaled `*100 >> 6` (the inverse of `ConvertVolumeValues`'
`<<6 /100`) and negated into DirectSound hundredths-of-dB; unused pairs pad with
`{0xFFFFFFFF, DSBVOLUME_MIN}`. Unallocated voice → `DSERR_INVALIDCALL`.

⚠️ Retail assembles volume 7's middle nibble from `VOLUME6_B7_4` (`VOLB & 0xF`)
instead of `VOLUME7_B7_4` (`(VOLB >> 16) & 0xF`) — contradicting its own
register layout and its own setter. That is a retail bug, not a hidden layout;
our implementation extracts the field the setter actually writes.

### `CEngine::EnableHeadphones(BOOL)`

XACT lock, then `IDirectSound_EnableHeadphones(engine->pDirectSound /*+0x10*/,
fEnabled)`. The −8 `this` adjustment lives only in the entry-point thunk (the
public `PXACTENGINE` points 8 bytes into retail's `CEngine`); every
`IXACTEngine_` entry does it, null-preserving. Our objects are their own
interface pointers, so the port's entry points cast directly.

### `CEngine::SetI3dl2Listener(const DSI3DL2LISTENER*, DWORD dwApply)`

Forwards to a **static** `CSoundSource::SetI3DL2Listener`, which copies the 12
dwords into a static `DS3DCALCI3DL2LISTENER` cache (dirty word |= 0xFFF), calls
`IDirectSound_SetI3DL2Listener(pDSound, pds3dl, dwApply)`, and — if immediate —
XACT's own static `CommitDeferredSettings`. The cache and commit feed retail
XACT's private 3D pipeline; our 3D pipeline is libdsound's, so the forward is
the entire recoverable behaviour (and the port's `SetListenerParameters` was
already exactly that forward).

### `CEngine::GetRealtimeData(XACT_REALTIME_AUDIO_DATA*)`

`IDirectSound_GetCaps` into `.DSoundCaps` (+0x40), bail on failure;
`IDirectSound_GetOutputLevels(pDSound, pData /* OutputLevels at +0 */, FALSE)`,
bail on failure; then `dwXactMemoryUsage = g_dwXACTEngineMemoryUsage` and the
three availability BYTEs from the engine — **another non-sequential trap**:
`b2DBuffers ← engine+0x80`, `b2DStreams ← engine+0x88`, `b3DBuffers ←
engine+0x84`. The port counts its `m_lstAvailable{2DBuffers,Streams,3DBuffers}`
lists and keeps a live `g_lXactMemoryUsage` in the allocator
(`ExQueryPoolBlockSize` makes free-side accounting possible).

### `CSoundBank::SelectVariation(DWORD, const XACT_SOUNDBANK_SELECT_VARIATION*)`

Retail walks its 5849-format variation tables: cue flag bit 3 → `E_INVALIDARG`;
`SOUND_VALUE` → weighted `SelectCueVariation(cue, flValue)` + an
"explicitly selected" bit; `SOUND_INDEX` → bounds-check against the table's
count (low 13 bits of its header dword), then write the current-index bits
(17:29, tagged 0x2000) and the cue's resolved sound word; NULL variation →
value-select with 1.0; then the same per-wave over the chosen sound's tracks;
sound still unresolved at the end → `E_FAIL`. **A cue with no variation table
accepts index 0 or any value select and rejects a nonzero index** — and since
the leak bank format (which our xactbld emits) has no variation tables at all
(one sound per cue, one wave per play event), that no-table path IS the port's
whole implementation, not a stub.

### `CSoundBank::GetSoundCueProperties(DWORD, XACT_SOUNDCUE_PROPERTIES*)`

Retail reads most fields off its 20-byte 5849-format sound entry: priority /
layer / category / track-count bytes; `lVolume = -((w4 & 0x1FF) << 4)`,
`lLFEVolume = -(((w4 >> 9) & 0x7F) * 50)`, `lI3DL2Volume = -(b0xF << 8)`
(volume, LFE and I3DL2 sends bit-packed into the entry); pitch and parametric
EQ words; the 3D block via a 40-byte-entry table; wave index from the sound's
wave-variation table or, flag-dependent, a per-track walk that keeps the wave
index only while every track agrees (else `XACT_WAVE_INDEX_UNUSED`), takes the
max loop count, and computes `dwLength` via `GetTrackLength` /
`GetDirectPlayLength`. The leak format keeps the same information elsewhere,
so the port fills each field from where it actually lives: sound entry,
3D-parameters block (incl. the extra volume pair as `lI3DL2Volume` when it
names the I3DL2 mixbin), and a raw walk of each track's event table
(Play/PlayWithPitchAndVolumeVariation → wave index with retail's consistency
rule, LoopStart → max loop count, SetVolume → authored volume). Not carried by
the leak format and left zero: per-sound pitch, parametric EQ, `dwLength`.

### `CSoundSource::SetMode` — implemented via libdsound

The recovered deferred-settings pattern above exists because retail XACT does
its own 3D math and pushes it on commit. In this port libdsound owns that
pipeline and its `SetMode` already honors `dwApply`, so the buffer-then-stream
forward is the equivalent implementation.

## Implemented — all ten

`SetPitch`, `GetStatus`, `SetFilter`, `GetProperties` (with
`IDirectSound{Buffer,Stream}_GetVoiceProperties` recovered into libdsound to
support it), `SetMode`, `EnableHeadphones`, `SetI3dl2Listener`,
`GetRealtimeData`, `SelectVariation`, `GetSoundCueProperties` — see the libxact
and libdsound commits.
