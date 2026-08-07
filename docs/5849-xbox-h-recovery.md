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

## Scoped follow-ups (5) — heavyweight write-side subsystems, no sample/title user

These are not stubs and not blocked on retail secrets; they are each a
self-contained subsystem the read-only / abstracted port never carried, and
implementing one faithfully is real design work against our own internals, not
a mechanical transcription. Recorded here so the next session starts from the
right place.

### Soundtrack write: `XAddSoundtrack`, `XAddSongToSoundtrack`

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

### Secondary utility drive: `XMountSecondaryUtilityDrive`, `XSwapUtilityDrives`, `XFormatSecondaryUtilityDrive`

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
