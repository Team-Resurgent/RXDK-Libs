// libxmv source manifest.
//
// XMV FMV playback rebuilt around the open XMV format (FFmpeg/VLC are the
// reference): a standalone container demuxer (src/xmvdemux.c) feeding a WMV2
// video decoder + PCM/ADPCM audio, behind the retail XMVDecoder_* API
// (src/xmvplay.c). The retail xmv.lib is used ONLY as a behavioural reference,
// not linked.
//
// src/ holds the active port: demuxer + XMVDecoder_* API + WMV2 I/P decoder
// (xmvcore wraps the leak I-frame kernel for keyframes; wmv2*.c is the ported
// WMV2 P-frame decoder). The leak decoder under decoder/ + inc/ is retained as
// a reference and built only for its keyframe (I-frame) entry points.

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
};

pub const xmv_c_sources = [_][]const u8{
    "libs/libxmv/src/xmvdemux.c",
    "libs/libxmv/src/xmvplay.c",
    "libs/libxmv/src/xmvcore.c",
    // WMV2 P-frame decode (FFmpeg-derived): VLC tables + reader + header parse.
    "libs/libxmv/src/wmv2_tables.c",
    "libs/libxmv/src/wmv2_vlc.c",
    "libs/libxmv/src/wmv2dec.c",
    "libs/libxmv/src/wmv2_mb.c",
    // Leak software video kernel (Phase 2 keyframe/I-frame decode). frontend.c +
    // backend.c carry MSVC __asm{} MMX (InverseDCT / YUV->YUY2) -- built >= -O2.
    "libs/libxmv/decoder/frontend.c",
    "libs/libxmv/decoder/backend.c",
    "libs/libxmv/decoder/bits.c",
    "libs/libxmv/decoder/huffman.c",
    "libs/libxmv/decoder/tables.c",
};

pub const slices = [_]Slice{
    .{ .name = "xmv", .is_cpp = false, .sources = &xmv_c_sources },
};
