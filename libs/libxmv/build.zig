const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xmv_sources = @import("sources.zig");

const X = "libs/libxmv";

// Include environment for the Xbox XMV decoder. The decoder/*.c pull <xtl.h>,
// <xdbg.h>, <xmv.h> + "decoder.h" + "..\inc\xmvformat.h"; site/bridge_xmv.h
// (force-included) pre-pulls the d3d8/dsound/xtl umbrella. Title-side, like
// libxgraphics -- reuses libxapi's NT/xtl header set + shared/include public
// headers (d3d8.h, dsound.h, xmv.h).
pub fn includeDirs() []const []const u8 {
    return &.{
        X ++ "/src",
        X ++ "/decoder",
        X ++ "/inc",
        X ++ "/site",
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

// -fasm-blocks: backend.c keeps its MSVC __asm{} MMX YUV->YUY2 converter verbatim
// (clang compiles it natively, like libxgraphics/libdsound). The bridge supplies
// the Win32/COM/CRT env.
const common_flags = [_][]const u8{
    "-ffreestanding",
    "-fno-stack-protector",
    "-fdata-sections",
    "-ffunction-sections",
    "-nostdinc",
    "-fms-extensions",
    "-fms-compatibility",
    "-fasm-blocks",
    "-fno-operator-names",
    "-fno-sanitize=undefined",
    "-Wno-everything",
    "-D_XAPI_",
    "-include",
    "picolibc.h",
    "-include",
    "libs/libxapi/site/profile.h",
    "-include",
    X ++ "/site/bridge_xmv.h",
};

pub fn cppFlags(b: *std.Build) []const []const u8 {
    const cpp = [_][]const u8{ "-std=c++17", "-fno-exceptions", "-fno-rtti", "-nostdinc++" };
    return std.mem.concat(b.allocator, []const u8, &.{ &cpp, &common_flags }) catch @panic("OOM");
}

pub fn cFlags(b: *std.Build) []const []const u8 {
    const c = [_][]const u8{"-std=c17"};
    return std.mem.concat(b.allocator, []const u8, &.{ &c, &common_flags }) catch @panic("OOM");
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

    // backend.c / frontend.c keep MSVC __asm{} MMX blocks that clang only lowers
    // at >= -O2 (same constraint as libxgraphics). Bump a Debug (-O0) build to -O2
    // so the asm-block decoder compiles; honour any higher level otherwise.
    const xmv_opt = if (std.mem.eql(u8, opt_flag, "-O0")) "-O2" else opt_flag;

    for (xmv_sources.slices) |slice| {
        const flags = if (slice.is_cpp) cppFlags(b) else cFlags(b);
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("xmv-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("xmv/{s}", .{slice.name}),
            .sources = slice.sources,
            .flags = flags,
            .include_dirs = includeDirs(),
            .opt_flag = xmv_opt,
            .is_cpp = slice.is_cpp,
        });
        all_outputs.appendSlice(allocator, batch.outputs) catch @panic("OOM");
        all_steps.append(allocator, batch.step) catch @panic("OOM");
    }

    const aggregate = b.allocator.create(std.Build.Step) catch @panic("OOM");
    aggregate.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "compile-xmv-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
