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
    // The WMA decoder is plain C99 from ffmpeg; it builds against picolibc alone, WITHOUT the
    // DirectSound bridge/precomp umbrella (see build.zig's wmaFlags).
    minimal_c: bool = false,
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
    // RXDK 5849 uplift: WMA decoder XMO (the leak has no WMA support at all).
    "libs/libdsound/dsound/wmaxmo.cpp",
    // dsac97.lib is an OBJLIB of dsound.lib (the AC97 codec hardware channel
    // driver); fold its single source in so libdsound is self-contained.
    "libs/libdsound/ac97/ac97.cpp",
};
pub const dsound_c_sources = [_][]const u8{
    "libs/libdsound/dsound/globals.c",
};

// Self-contained ffmpeg WMA v1/v2 decoder (from XBMC4Xbox's mplayer/libavcodec), PC-verified
// (0.99998 correlation vs ffmpeg), plus the ASF reader and streaming front end the XMO decoders
// need. Lives here rather than in libxact because 5849's dsound.lib owns the WMA decoder and
// xact.lib calls into it -- and a title can stream WMA without linking XACT at all.
pub const dsound_wma_sources = [_][]const u8{
    "libs/libdsound/wma/wmadec.c",
    "libs/libdsound/wma/fft.c",
    "libs/libdsound/wma/mdct.c",
    "libs/libdsound/wma/get_bits.c",
    "libs/libdsound/wma/wmabridge.c",
    "libs/libdsound/wma/wmastream.c",
    "libs/libdsound/wma/asf.c",
};

pub const slices = [_]Slice{
    .{ .name = "dsound", .is_cpp = true, .sources = &dsound_cpp_sources },
    .{ .name = "dsound-c", .is_cpp = false, .sources = &dsound_c_sources },
    .{ .name = "wma", .is_cpp = false, .minimal_c = true, .sources = &dsound_wma_sources },
};
