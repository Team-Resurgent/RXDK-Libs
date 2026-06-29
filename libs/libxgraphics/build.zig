const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xg_sources = @import("sources.zig");

const XG = "libs/libxgraphics";

// Include environment for the Xbox xgraphics library. Component dirs + inc, the
// public xgraphics/xgmath/d3d8 headers in shared/include, plus libxapi's NT/xtl
// header set (title-side code). See site/bridge_xgraphics.h (force-included).
pub fn includeDirs() []const []const u8 {
    return &.{
        XG ++ "/inc",
        XG ++ "/pushbuffer",
        XG ++ "/shadeasm",
        XG ++ "/swizzler",
        XG ++ "/math",
        XG ++ "/misc",
        XG ++ "/dxtc",
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

// compile_c.addBatch adds -march=pentium3 / -target / -c / -o + the opt flag.
// -fasm-blocks: the swizzler / xgmath / dxtc keep their hand-written MSVC
// __asm{} MMX/SSE blocks; clang compiles those natively under -fasm-blocks
// (kept verbatim -- no transcription). The bridge is force-included for the
// NT/xtl/D3D + xgraphics-header environment and the MSVC-CRT shims.
pub fn cppFlags(_: *std.Build) []const []const u8 {
    return &.{
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
        "-fasm-blocks",
        "-fno-sanitize=undefined",
        "-Wno-everything",
        "-D_XAPI_",
        "-include",
        "picolibc.h",
        "-include",
        "libs/libxapi/site/profile.h",
        "-include",
        XG ++ "/site/bridge_xgraphics.h",
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

    // Force >= -O2: swizzler.cpp's large MASM asm-blocks (XGSwizzleRect /
    // XGUnswizzleRect bpp==4 loops) reference ~14 live operands and clang's
    // asm-block register allocator can only satisfy them with optimization on
    // (a Debug -O0 build fails with "inline assembly requires more registers").
    // xgraphics is asm-heavy perf code, so building it optimized is correct.
    const xg_opt = if (std.mem.eql(u8, opt_flag, "-O0")) "-O2" else opt_flag;

    for (xg_sources.slices) |slice| {
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("xgraphics-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("xgraphics/{s}", .{slice.name}),
            .sources = slice.sources,
            .flags = cppFlags(b),
            .include_dirs = includeDirs(),
            .opt_flag = xg_opt,
            .is_cpp = slice.is_cpp,
        });
        all_outputs.appendSlice(allocator, batch.outputs) catch @panic("OOM");
        all_steps.append(allocator, batch.step) catch @panic("OOM");
    }

    const aggregate = b.allocator.create(std.Build.Step) catch @panic("OOM");
    aggregate.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "compile-xgraphics-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
