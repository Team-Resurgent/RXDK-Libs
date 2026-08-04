// libxfont source manifest — the Xbox XFONT bitmap-font text-rendering library,
// ported from the May-2020 leak (private/windows/directx/dxg/xfont/library).
// Bitmap fonts only: TrueType (truetype.cpp + the third-party scaler engine
// under xfont/scaler/) and disk-file font loading (the CreateFileA/ReadFile
// path originally in bitmap.cpp) are both deliberately out of scope for this
// pass -- see libs/libxfont/xfont/*.c(pp) file headers for what was trimmed
// and why.

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
    extra_flags: []const []const u8 = &.{},
    // The scan converter predates the Windows header umbrella and declares its own
    // ULONG/LPSTR/LARGE_INTEGER etc., which collide with libxapi's. It builds against
    // picolibc alone, without the force-included RXDK bridge -- see build.zig.
    minimal_c: bool = false,
};

pub const xfont_c_sources = [_][]const u8{
    "libs/libxfont/xfont/xfont.c",
};

pub const xfont_cpp_sources = [_][]const u8{
    "libs/libxfont/xfont/bitmap.cpp",
    "libs/libxfont/xfont/painttext.cpp",
    "libs/libxfont/xfont/truetype.cpp",
};

// The scan converter, verbatim from the leak's scaler/ (its xbox/sources SOURCES= set).
pub const scaler_sources = [_][]const u8{
    "libs/libxfont/scaler/fnterr.c",
    "libs/libxfont/scaler/fontmath.c",
    "libs/libxfont/scaler/fscaler.c",
    "libs/libxfont/scaler/fsglue.c",
    "libs/libxfont/scaler/interp.c",
    "libs/libxfont/scaler/scale.c",
    "libs/libxfont/scaler/scanlist.c",
    "libs/libxfont/scaler/scbitmap.c",
    "libs/libxfont/scaler/scendpt.c",
    "libs/libxfont/scaler/scentry.c",
    "libs/libxfont/scaler/scline.c",
    "libs/libxfont/scaler/scmemory.c",
    "libs/libxfont/scaler/scspline.c",
    "libs/libxfont/scaler/sfntaccs.c",
    "libs/libxfont/scaler/subpixel.c",
    "libs/libxfont/scaler/sbit.c",
};

pub const slices = [_]Slice{
    .{ .name = "c", .is_cpp = false, .sources = &xfont_c_sources, .extra_flags = &.{"-DXFONT_TRUETYPE"} },
    .{ .name = "cpp", .is_cpp = true, .sources = &xfont_cpp_sources, .extra_flags = &.{"-DXFONT_TRUETYPE"} },
    .{ .name = "scaler", .is_cpp = false, .minimal_c = true, .sources = &scaler_sources },
};
