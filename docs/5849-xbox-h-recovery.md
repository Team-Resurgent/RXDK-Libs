# Recovering the missing Xbox.h APIs from `xapilib.lib`

Nine public `Xbox.h` APIs are declared in the 5849 header and shipped in the
retail `xapilib.lib` but implemented nowhere in our port. This file records
what was read out of the binary, the same way `5849-xact-api-recovery.md` does
for XACT.

## Method

Same recipe as the XACT recovery: find the archive member with the symbol,
extract it in Python (60-byte header, decimal ASCII size at offset 48), then
`dumpbin /DISASM:BYTES` from PowerShell. The nine live in five members:
`xgetautologon.obj`, `xapiheap.obj`, `filehops.obj`, `pathmisc.obj`, and
`soundtrack.obj` (the last in `xsndtrk.lib`, not `xapilib.lib`).

## Implemented (4)

### `XGetAutoLogonFlag()`

Reads misc-flags EEPROM setting (`ExQueryNonVolatileSetting(XC_MISC_FLAGS)`,
index 0x11). Returns `XC_AUTO_LOGON_ALLOWED` (1) when bit `0x4` is clear,
`XC_AUTO_LOGON_NOT_ALLOWED` (2) when it is set or the read fails. In
`k32/xvalue.c` beside the other `XC_`-setting readers.

### `XSetAttributesOnHeapAlloc(pv, dw)` / `XGetAttributesOnHeapAlloc(pv)`

The Xbox `HEAP_ENTRY` carries an 8-byte reserved slot immediately before the
returned user pointer; these are its accessors — `mov [ecx-8], eax` and
`mov eax, [eax-8]`, i.e. `((PDWORD)pBaseAddress)[-2]`. In `k32/xapiheap.c`.

### `XGetFilePhysicalSortKey(hFile)`

Returns a key ordering files by physical position so a title can batch reads
seek-efficiently. References the file object
(`ObReferenceObjectByHandle` + `IoFileObjectType`), asks the volume its FS name
(`IoQueryVolumeInformation` / `FileFsAttributeInformation`), and — because both
FS names are four chars — compares the name as one dword: `'XTAF'` → the FATX
starting cluster (0 for a directory, flagged by bit 0 of the file context),
`'XFDG'` → the XDVDFS starting sector. Anything else →
`STATUS_INVALID_DEVICE_REQUEST`. The file system's per-file context has no
public type in our tree, so the two fields are read at the fixed offsets the
disassembly uses (FATX flags +0, cluster +0x1C; XDVDFS sector +0). In
`k32/filehops.c`.

## Implemented (5 more) — the write-side subsystems

Originally scoped as follow-ups; implemented 2026-08-07 for full parity. Each is
a self-contained subsystem the read-only / abstracted port never carried, so
these are faithful ports against our own internals, not mechanical
transcriptions. **⚠️ Both mutate on-disk structures and neither has a sample or
kit test, so the runtime path is HW-only.** What *is* verified in-repo: they
build with the correct export decorations; the secondary-drive DB scheme is
reconciled with our existing primary allocator (which already maintained the
same on-disk database); and the ST.DB format logic is round-trip-verified on the
host (`scratchpad/stdb_roundtrip.c`) — write-side page math + struct writes
decode correctly through the read side's page math. The soundtrack writer's song
length and title come from the same WMA decoder call retail makes, so no field is
approximated (see the soundtrack section below).

### Soundtrack write: `XAddSoundtrack`, `XAddSongToSoundtrack` — DONE (`k32/xsndtrkw.c`)

Implemented in a **separate translation unit** so a title using only the read
side never pulls it. `XAddSoundtrack` allocates a descriptor (page `StCount+1`),
fills `STDB_STDESC` (sig `0x021371`, Id from `NextStId`, name), records the Id in
`StBlocks[]`, creates the per-soundtrack `MUSIC\%04x` directory, and writes the
descriptor page *before* the header (so the header never points at an unwritten
descriptor). `XAddSongToSoundtrack` locates the soundtrack, allocates a song Id
(`(soundtrackId<<16)|seq`, matching the read side's `%04x\%08x.WMA`), copies the
WMA into `MUSIC\%04x\`, and inserts the song into a list block (page
`block+1+MAX_SOUNDTRACKS`), allocating a new block on each 6-song boundary.

**Song length + title — via the WMA decoder, exactly like retail.** The
disassembly settled the earlier "unreachable duration" caveat: retail's
`soundtrack.obj` drives the WMA file decoder object, taking the play length from
its vtable slot `+0x28` (`GetFileHeader`, `WMAXMOFileHeader.dwDuration`) and, when
the caller passes no name, the Title tag from `+0x2C`
(`GetFileContentDescription`), defaulting an empty tag to `"Unknown"`. The port's
`WmaCreateDecoderEx` (libdsound) hands back an `XWmaFileMediaObject` whose vtable
is bit-identical (`Release` `+0x4`, `GetFileHeader` `+0x28`,
`GetFileContentDescription` `+0x2C`), so `xsndtrkw.c` calls exactly those slots.
The decoder *borrows* the already-open source handle
(`CWmaMediaObject::InitializeFile` leaves `m_fCloseFile` FALSE for a handle it did
not open), so the caller keeps ownership — matching retail, which passes the same
handle to the decoder and to the copy. This is the one place the write side
reaches outside libxapi, to `_WmaCreateDecoderEx@32` in libdsound — retail's own
cross-library model, since `xsndtrk.lib` shipped separately from `xapilib.lib`. A
title using only the read side never pulls the object and never needs libdsound.
Because it is the same decoder call retail makes, the length and title are now
faithful-by-construction, not approximated.

### Soundtrack write — original recovery notes

Our `k32/xsndtrk.c` is the **enumeration (read) side only** — it parses
`TDATA\FFFE0000\MUSIC\ST.DB` through its own helpers (`XapiReadFromStDb`,
`XapiOpenStDbAndReadHeader`, `XapiGetNextSoundtrack`). The retail write side is
a ~1000-line family in `soundtrack.obj` that has no counterpart here:
`XapipWriteToSoundtrackDb`, `XapipUpdateSoundtrackDbHeader`,
`XapipFindSoundtrackBlock`, `XapipSoundtrackFindNewListBlock`,
`XapipSoundtrackSeek{,ToListSegment,ByPage}`, `XapipCopySongToMusicDirectory`,
`XapipSoundtrackNewSongID`, `XapiBeginUsingSoundtracks`/`EndUsingSoundtracks`.
It manages on-disk block allocation of the DB and copies the WMA into the MUSIC
directory. A faithful port means recovering that block manager, on top of the
STDB structures we already have in `xboxp.h`.

### Secondary utility drive: `XMountSecondaryUtilityDrive`, `XSwapUtilityDrives`, `XFormatSecondaryUtilityDrive` — DONE (`k32/pathmisc.c`)

The reconciliation turned out small: our `XapiSelectCachePartition` **already
maintains** the exact on-disk cache-partition database retail uses (sector
`XBOX_CACHE_DB_SECTOR_INDEX`, sig `0x97315286`, 12-byte `X_CACHE_DB_ENTRY`
records) — it reads, validates, MRU-slides, and writes it back. The only missing
piece was recording which DB slot the Z: entry landed in, so the secondary APIs
can find Z:'s partition: a one-line `g_iZDriveDBIndex = iNewDBIndex;` that changes
no existing behavior. `XMountSecondaryUtilityDrive` then reads the DB, finds a
free partition that is not Z:'s, records it in the least-recently-used slot,
formats it, and links `\??\N:`. `XSwapUtilityDrives` swaps only the two records'
`nCacheIndex` fields and re-points the Z:/N: symlinks. `XFormatSecondaryUtilityDrive`
resolves `\??\N:` and reformats in place (identical shape to
`XFormatUtilityDrive`). ⚠️ The spec's premise that the DB lives at partition0
sector 0 was wrong — it is byte offset `0x800` (sector 4), and the end signature
is at `0x1F8`; our existing code already had both right.

### Secondary utility drive — original recovery notes

Retail mounts a second cache partition as `N:` by reading the raw refurb-info
sector (partition0 sector 0, signature `0x97315286`, MBR magic `0xAA55`,
version 2), walking a 0x28-entry cache-partition database of 12-byte records,
and tracking the chosen Z: and N: slots in `g_iZDriveDBIndex` /
`g_iNDriveDBIndex`. `XMount` finds/marks a free N slot and symlinks `\??\N:`;
`XSwap` exchanges the two DB records; `XFormat` formats N. Our primary
`XMountUtilityDrive` deliberately uses a different allocator
(`XapiSelectCachePartition`) that does **not** maintain the refurb-sector DB or
those indices, so the secondary path cannot be layered on it as-is. Faithful
support means either reviving retail's refurb-sector DB scheme (and reconciling
it with our primary allocator) or designing the Z/N pairing onto
`XapiSelectCachePartition`. All the leaf helpers it needs already exist
(`XapiFormatFATVolumeEx`, `XapiValidateDiskPartitionEx`,
`HalDiskCachePartitionCount`, `CacheDriveFormat`, `XapiHardDisk`); only the DB
bookkeeping is missing.
