const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xfont_sources = @import("sources.zig");

const XFONT = "libs/libxfont";

// Include environment for the Xbox XFONT bitmap-font library. The component
// dir + the public xfont/xfontformat/d3d8 headers in shared/include, plus
// libxapi's NT/xtl header set (XFONT is title-side code that links against
// the public d3d8 + CRT surface, same shape as libd3dx8). See
// site/bridge_xfont.h, force-included before each TU's own includes.
pub fn includeDirs() []const []const u8 {
    return &.{
        XFONT ++ "/xfont",
        XFONT ++ "/scaler",
        "shared/include",
        "libs/libxapi/internal",
        "libs/libxapi/internal/shims",
        "libs/libxapi/support/inc",
        "libs/libxapi/support/inc/ntos",
        "libs/libxapi/nt",
        "libs/libxapi/rtl/inc",
        "libs/libxapi/port",
        "libs/libxapi/site",
        "build/generated",
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
    };
}

// Unlike libd3d8/libdsound (both /Gz kernel-runtime code needing
// -fdefault-calling-conv=stdcall + a cdecl_libc.h to pin bare CRT calls back
// to __cdecl), XFONT's vendor source already tags every public entry point
// with an explicit __stdcall/__fastcall, and its untagged internal helpers +
// CRT calls (malloc/free/memmove/ZeroMemory) are all self-consistent under
// the compiler's plain i386 default (__cdecl) -- same shape as libd3dx8
// (d3dx.mk: 386_STDCALL=0). So: no -fdefault-calling-conv=stdcall, no
// cdecl_libc.h.
const force_includes = [_][]const u8{
    "-include",
    "picolibc.h",
    "-include",
    "libs/libxapi/site/profile.h",
    "-include",
    XFONT ++ "/site/bridge_xfont.h",
};

pub fn cppFlags(b: *std.Build) []const []const u8 {
    const base = [_][]const u8{
        "-std=c++17",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fdata-sections",
        "-ffunction-sections",
        "-fno-exceptions",
        "-fno-rtti",
        "-nostdinc",
        "-nostdinc++",
        "-fms-extensions",
        "-fms-compatibility",
        "-fno-sanitize=undefined",
        "-fno-builtin",
        "-Wno-everything",
        "-D_XAPI_",
    };
    return std.mem.concat(b.allocator, []const u8, &.{ &base, &force_includes }) catch @panic("OOM");
}

pub fn cFlags(b: *std.Build) []const []const u8 {
    const base = [_][]const u8{
        "-std=c17",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fdata-sections",
        "-ffunction-sections",
        "-nostdinc",
        "-fms-extensions",
        "-fms-compatibility",
        "-fno-sanitize=undefined",
        "-fno-builtin",
        "-Wno-everything",
        "-D_XAPI_",
    };
    return std.mem.concat(b.allocator, []const u8, &.{ &base, &force_includes }) catch @panic("OOM");
}

// Minimal C environment for the third-party TrueType scan converter: picolibc +
// freestanding only, WITHOUT the RXDK bridge/profile force-includes. The scaler
// declares its own ULONG/LPSTR/LARGE_INTEGER and friends (fscdefs.h/fsconfig.h),
// which redefine libxapi's -- the leak built it as its own library (xfonttt) for
// exactly that reason.
pub fn scalerFlags(b: *std.Build) []const []const u8 {
    const base = [_][]const u8{
        "-std=c17",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fdata-sections",
        "-ffunction-sections",
        "-nostdinc",
        "-fms-extensions",
        "-fms-compatibility",
        "-fno-sanitize=undefined",
        "-fno-builtin",
        "-Wno-everything",
        "-include",
        "picolibc.h",
    };
    return b.allocator.dupe([]const u8, &base) catch @panic("OOM");
}

// Include path for the scaler slice: its own directory plus picolibc. Deliberately
// NOT libxapi's -- letting those on the path is what drags in ntdef.h and reopens
// the type collisions this slice exists to avoid.
pub fn scalerIncludeDirs() []const []const u8 {
    return &.{
        XFONT ++ "/site/scaler_shim",
        XFONT ++ "/scaler",
        "build/generated",
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
    };
}

pub const ObjectBatch = struct {
    step: *std.Build.Step,
    outputs: []const std.Build.LazyPath,
};

pub fn addAllObjects(
    b: *std.Build,
    xbox_target: @TypeOf(@import("../../build/xbox_target.zig")),
    opt_flag: []const u8,
) ObjectBatch {
    const allocator = b.allocator;
    var all_outputs = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    var all_steps = std.ArrayListUnmanaged(*std.Build.Step).empty;

    for (xfont_sources.slices) |slice| {
        const base_flags = if (slice.is_cpp) cppFlags(b) else if (slice.minimal_c) scalerFlags(b) else cFlags(b);
        const flags = if (slice.extra_flags.len == 0) base_flags else
            std.mem.concat(b.allocator, []const u8, &.{ base_flags, slice.extra_flags }) catch @panic("OOM");
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("xfont-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("xfont/{s}", .{slice.name}),
            .sources = slice.sources,
            .flags = flags,
            .include_dirs = if (slice.minimal_c) scalerIncludeDirs() else includeDirs(),
            .opt_flag = opt_flag,
            .is_cpp = slice.is_cpp,
        });
        all_outputs.appendSlice(allocator, batch.outputs) catch @panic("OOM");
        all_steps.append(allocator, batch.step) catch @panic("OOM");
    }

    const aggregate = b.allocator.create(std.Build.Step) catch @panic("OOM");
    aggregate.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "compile-xfont-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
