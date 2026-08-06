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

### `CSoundSource::GetProperties(XACT_SOUNDSOURCE_PROPERTIES*)` — partially read

```
lock;
pProps->[0] = (DWORD)(WORD)this+0x12;          // a 16-bit field, zero-extended
hr = IDirectSoundBuffer_GetVoiceProperties(this+0x1C /* pBuffer */, &pProps[4]);
if (SUCCEEDED(hr) && (this+0x14 & 2)) { ...reads this+0xEC... }
```

Two things to settle before implementing it. It calls `GetVoiceProperties` on the
**buffer unconditionally** — there is no stream branch, unlike its neighbours — so
what a stream-backed source is meant to return is still open. And the tail behind
the `& 2` flag test (3D, most likely) has not been read to the end.

Not implemented for that reason: the recovered part is the easy half, and
guessing the rest would defeat the point of reading the binary at all.

## Implemented so far

`SetPitch`, `GetStatus`, `SetFilter` — see the libxact commits.

## Still to read

`SelectVariation`, `GetSoundCueProperties`, `GetRealtimeData`,
`SetI3dl2Listener`, plus the tail of `GetProperties` and the `CEngine` side of
`EnableHeadphones` (whose entry point needs the −8 `this` adjustment above).
