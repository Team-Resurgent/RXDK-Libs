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

## Implemented so far

`SetPitch`, `GetStatus`, `SetFilter`, `GetProperties` (with
`IDirectSound{Buffer,Stream}_GetVoiceProperties` recovered into libdsound to
support it) — see the libxact and libdsound commits.

## Still to read

`SelectVariation`, `GetSoundCueProperties`, `GetRealtimeData`,
`SetI3dl2Listener`, and the `CEngine` side of `EnableHeadphones` (whose entry
point needs the −8 `this` adjustment above).
