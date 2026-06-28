// libd3d8 source manifest — the Xbox D3D8 (NV2A) driver ported from the
// May-2020 leak (private/windows/directx/dxg/d3d8/se). Mirrors libxapi/sources.zig.
// Every .cpp #includes se/precomp.hpp first, which sets up the kernel/xtl/NV2A
// include environment (see libs/libd3d8/inc for the external deps it pulls).

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
};

// The full d3d8.lib SOURCES= set (se/sources.inc), exact on-disk filenames.
pub const se_sources = [_][]const u8{
    "libs/libd3d8/se/block.cpp",
    "libs/libd3d8/se/buffer.cpp",
    "libs/libd3d8/se/clear.cpp",
    "libs/libd3d8/se/combiner.cpp",
    "libs/libd3d8/se/d3dbase.cpp",
    "libs/libd3d8/se/d3ddev.cpp",
    "libs/libd3d8/se/debug.cpp",
    "libs/libd3d8/se/drawprim.cpp",
    "libs/libd3d8/se/dumper.cpp",
    "libs/libd3d8/se/dxgcreate.cpp",
    "libs/libd3d8/se/enum.cpp",
    "libs/libd3d8/se/floatmath.cpp",
    "libs/libd3d8/se/globals.cpp",
    "libs/libd3d8/se/lazy.cpp",
    "libs/libd3d8/se/math.cpp",
    "libs/libd3d8/se/memory.cpp",
    "libs/libd3d8/se/mpcore.cpp",
    "libs/libd3d8/se/mpdac.cpp",
    "libs/libd3d8/se/mphal.cpp",
    "libs/libd3d8/se/mpintr.cpp",
    "libs/libd3d8/se/mpmode.cpp",
    "libs/libd3d8/se/overlay.cpp",
    "libs/libd3d8/se/patch.cpp",
    "libs/libd3d8/se/PatchBackend.cpp",
    "libs/libd3d8/se/PatchConst.cpp",
    "libs/libd3d8/se/PatchUtil.cpp",
    "libs/libd3d8/se/pixeljar.cpp",
    "libs/libd3d8/se/present.cpp",
    "libs/libd3d8/se/pshader.cpp",
    "libs/libd3d8/se/pusher.cpp",
    "libs/libd3d8/se/pushres.cpp",
    "libs/libd3d8/se/rdi.cpp",
    "libs/libd3d8/se/resource.cpp",
    "libs/libd3d8/se/shadersnapshot.cpp",
    "libs/libd3d8/se/state.cpp",
    "libs/libd3d8/se/stats.cpp",
    "libs/libd3d8/se/surface.cpp",
    "libs/libd3d8/se/texture.cpp",
    "libs/libd3d8/se/vshader.cpp",
};

pub const slices = [_]Slice{
    .{ .name = "se", .is_cpp = true, .sources = &se_sources },
};
