// libxgraphics source manifest — the Xbox xgraphics helper library ported from
// the May-2020 leak (private/windows/directx/dxg/xgraphics, the `xbox` build).
// Mirrors libd3dx8/sources.zig.
//
// xgraphics.lib (xbox) = pushbuffer + shadeasm + swizzler + math (xgmath) +
// misc + dxtc (the leak's link/sources.inc also bundles xfont, a separate
// TrueType library outside the xgraphics tree -- not ported here). Title-side,
// default-__cdecl; each TU is force-included with site/bridge_xgraphics.h. The
// hand-written MASM __asm{} blocks compile via clang -fasm-blocks (see build.zig).

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
};

// --- swizzler: XGSwizzle*/XGUnswizzle*/XGIsSwizzledFormat/XGBytesPerPixelFromFormat ---
pub const swizzler_sources = [_][]const u8{
    "libs/libxgraphics/swizzler/swizzler.cpp",
};

// --- pushbuffer ---
pub const pushbuffer_sources = [_][]const u8{
    "libs/libxgraphics/pushbuffer/pushbuffer.cpp",
};

// --- math (xgmath): vector/matrix helpers ---
pub const math_sources = [_][]const u8{
    "libs/libxgraphics/math/xgmath.cpp",
};

// --- misc: debug, header, surface-to-file, write-pixel-rect ---
pub const misc_sources = [_][]const u8{
    "libs/libxgraphics/misc/debug.cpp",
    "libs/libxgraphics/misc/header.cpp",
    "libs/libxgraphics/misc/surfacetofile.cpp",
    "libs/libxgraphics/misc/writexpr.cpp",
};

// --- dxtc: S3TC (DXT1/3/5) compress + quantize ---
pub const dxtc_sources = [_][]const u8{
    "libs/libxgraphics/dxtc/s3_intrf.cpp",
    "libs/libxgraphics/dxtc/s3_quant.cpp",
};

// --- shadeasm: the pixel/vertex shader assembler + validator (NOT stdafx.cpp) ---
pub const shadeasm_sources = [_][]const u8{
    "libs/libxgraphics/shadeasm/cd3dxstack.cpp",
    "libs/libxgraphics/shadeasm/cd3dxassembler.cpp",
    "libs/libxgraphics/shadeasm/preprocessor.cpp",
    "libs/libxgraphics/shadeasm/api.cpp",
    "libs/libxgraphics/shadeasm/valbase.cpp",
    "libs/libxgraphics/shadeasm/pshdrval.cpp",
    "libs/libxgraphics/shadeasm/vshdrval.cpp",
    "libs/libxgraphics/shadeasm/pixelshader.cpp",
};

pub const slices = [_]Slice{
    .{ .name = "swizzler", .is_cpp = true, .sources = &swizzler_sources },
    .{ .name = "pushbuffer", .is_cpp = true, .sources = &pushbuffer_sources },
    .{ .name = "math", .is_cpp = true, .sources = &math_sources },
    .{ .name = "misc", .is_cpp = true, .sources = &misc_sources },
    .{ .name = "dxtc", .is_cpp = true, .sources = &dxtc_sources },
    .{ .name = "shadeasm", .is_cpp = true, .sources = &shadeasm_sources },
};
