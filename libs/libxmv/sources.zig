// libxmv source manifest.
//
// XMV FMV playback rebuilt around the open XMV format (FFmpeg/VLC are the
// reference): a standalone container demuxer (xmvplay/xmvdemux.c) feeding a
// WMV2 video decoder + PCM/ADPCM audio, behind the retail XMVDecoder_* API
// (xmvplay/xmvplay.c). The retail xmv.lib is used ONLY as a behavioural
// reference, not linked.
//
// PHASE 1 (now): demuxer + API + placeholder render -- proves retail container
// parsing end to end. PHASE 2: port FFmpeg's WMV2 decoder (wmv2/msmpeg4/
// mpegvideo). PHASE 3: PCM audio -> DirectSound stream. See [[libxmv-port]].
//
// The leak decoder under decoder/ + inc/ + src/ is retained as a reference for
// the OLDER XMV format/codec (NOT built -- wrong container generation, and it
// would collide with xmvplay's XMVDecoder_* exports).

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
};

pub const xmv_c_sources = [_][]const u8{
    "libs/libxmv/xmvplay/xmvdemux.c",
    "libs/libxmv/xmvplay/xmvplay.c",
    "libs/libxmv/xmvplay/xmvcore.c",
    // WMV2 P-frame decode (FFmpeg-derived): VLC tables + reader + header parse.
    "libs/libxmv/xmvplay/wmv2_tables.c",
    "libs/libxmv/xmvplay/wmv2_vlc.c",
    "libs/libxmv/xmvplay/wmv2dec.c",
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
