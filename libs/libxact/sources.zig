// libxact source manifest -- the Xbox XACT runtime audio engine (xacteng.lib)
// ported from the May-2020 leak (private/windows/directx/xact/runtime/engine).
//
// TITLE-SIDE library: it drives playback through DirectSound's PUBLIC API
// (IDirectSoundBuffer/IDirectSoundStream, DirectSoundCreate, WAVEFORMATEX,
// DSMIXBINS ...) plus the xboxkrnl timer/DPC/IRQL + pool primitives. It does NOT
// poke APU/PCI hardware, so its bridge (site/bridge_xact.h) is modeled on the
// title-side libxmv, NOT the kernel libdsound. The engine's helper machinery
// (refcount/ntlist/debug/memmgr/macros/drvhlp) lives in inc/ as header-only
// support.
//
// engine/ is the exact SOURCES= set from the leak engine/sources (all C++).
// common/memmgr.cpp is added from the leak runtime/common so libxact defines its
// OWN internal symbols (XactMemAlloc/XactMemFree -- the retail path is just
// ExAllocatePoolWithTag/ExFreePool + memset, backing the engine's new/delete).
// The other runtime/common .cpp are NOT staged: debug.cpp is entirely under
// #ifdef USEDPF (empty in the forced-retail build, and DwDbg* is never called),
// drvhlp.cpp is empty in retail (its and/or helpers are unreferenced), and the
// wavbndlr reader.cpp is the DSOUND-common file-authoring path (CStdFileStream/
// dscommon.h) the engine never uses -- RegisterWaveBank takes a caller buffer, so
// CWaveBankReader is never referenced. After memmgr, the only symbols libxact
// leaves undefined are dsound/kernel/xapi/libc exports resolved at title link.

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
    // Minimal-C slices (the ported ffmpeg WMA decoder) build against picolibc only, WITHOUT the
    // XACT engine's bridge_xact.h/COM/DPF force-includes (which would clash with the decoder's shim).
    minimal_c: bool = false,
};

pub const xact_cpp_sources = [_][]const u8{
    "libs/libxact/engine/xactapi.cpp",
    "libs/libxact/engine/uplift5849.cpp",
    "libs/libxact/engine/wmaplaylist.cpp",
    "libs/libxact/engine/common.cpp",
    "libs/libxact/engine/engine.cpp",
    "libs/libxact/engine/soundbank.cpp",
    "libs/libxact/engine/soundsource.cpp",
    "libs/libxact/engine/wavebank.cpp",
    // RXDK 5849 uplift: playback of a wave that is streamed rather than resident (the leak's
    // engine had no such path -- see CStreamedWave).
    "libs/libxact/engine/streamwave.cpp",
    "libs/libxact/engine/cue.cpp",
    "libs/libxact/engine/sequencer.cpp",
    // Machinery implementation (memory manager) so libxact defines XactMemAlloc/
    // XactMemFree itself instead of leaving them as title-link externals.
    "libs/libxact/common/memmgr.cpp",
};


pub const slices = [_]Slice{
    .{ .name = "xact", .is_cpp = true, .sources = &xact_cpp_sources },
};
