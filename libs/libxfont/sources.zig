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
};

pub const xfont_c_sources = [_][]const u8{
    "libs/libxfont/xfont/xfont.c",
};

pub const xfont_cpp_sources = [_][]const u8{
    "libs/libxfont/xfont/bitmap.cpp",
    "libs/libxfont/xfont/painttext.cpp",
};

pub const slices = [_]Slice{
    .{ .name = "c", .is_cpp = false, .sources = &xfont_c_sources },
    .{ .name = "cpp", .is_cpp = true, .sources = &xfont_cpp_sources },
};
