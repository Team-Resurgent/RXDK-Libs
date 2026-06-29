// libdsound source manifest — the Xbox core DirectSound library (the MCPX APU
// driver) ported from the May-2020 leak (private/windows/directx/dsound/dsound).
// Mirrors libd3dx8/libxgraphics. The codecs/ tree (WMA/voxware/adpcm, ~163K
// lines) is OUT of scope -- separate codec libs not needed for PCM playback.
//
// The dsound/ TUs are thin wrappers that #include "..\common\<name>.cpp"; each
// pulls dsound/dsoundi.h (precomp). Kernel+title hybrid (it pokes APU/PCI HW),
// so site/bridge_dsound.h sets up the NTOS-runtime + COM/d3dx8 + CRT-shim env.
// The few __asm sites compile via clang -fasm-blocks (see build.zig).

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
};

// The dsound/sources SOURCES= set (exact filenames). globals.c is C.
pub const dsound_cpp_sources = [_][]const u8{
    "libs/libdsound/dsound/ac97xmo.cpp",
    "libs/libdsound/dsound/cipher.cpp",
    "libs/libdsound/dsound/dsperf.cpp",
    "libs/libdsound/dsound/dsapi.cpp",
    "libs/libdsound/dsound/dscommon.cpp",
    "libs/libdsound/dsound/dsmath.cpp",
    "libs/libdsound/dsound/dspdma.cpp",
    "libs/libdsound/dsound/epdsp.cpp",
    "libs/libdsound/dsound/gpdsp.cpp",
    "libs/libdsound/dsound/heap.cpp",
    "libs/libdsound/dsound/hrtf.cpp",
    "libs/libdsound/dsound/i3dl2.cpp",
    "libs/libdsound/dsound/mcpapu.cpp",
    "libs/libdsound/dsound/mcpbuf.cpp",
    "libs/libdsound/dsound/mcpstrm.cpp",
    "libs/libdsound/dsound/mcpvoice.cpp",
    "libs/libdsound/dsound/mcpxcore.cpp",
    "libs/libdsound/dsound/wavexmo.cpp",
    // dsac97.lib is an OBJLIB of dsound.lib (the AC97 codec hardware channel
    // driver); fold its single source in so libdsound is self-contained.
    "libs/libdsound/ac97/ac97.cpp",
};
pub const dsound_c_sources = [_][]const u8{
    "libs/libdsound/dsound/globals.c",
};

pub const slices = [_]Slice{
    .{ .name = "dsound", .is_cpp = true, .sources = &dsound_cpp_sources },
    .{ .name = "dsound-c", .is_cpp = false, .sources = &dsound_c_sources },
};
