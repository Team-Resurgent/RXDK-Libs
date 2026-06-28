const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const d3d8_sources = @import("sources.zig");

const D3D8 = "libs/libd3d8";

// Include environment for the Xbox D3D8 (NV2A) driver. The se/ sources +
// libd3d8/inc external deps, plus libxapi's internal NT/xtl headers (D3D8 runs in
// the kernel runtime and shares that header set). See site/bridge_d3d8.h.
pub fn includeDirs() []const []const u8 {
    return &.{
        D3D8 ++ "/se",
        D3D8 ++ "/inc",
        "shared/include",
        "libs/libxapi/internal",
        "libs/libxapi/internal/shims",
        "libs/libxapi/support/inc",
        "libs/libxapi/support/inc/ntos",
        "libs/libxapi/k32",
        "libs/libxapi/k32/inc",
        "libs/libxapi/nt",
        "libs/libxapi/rtl/inc",
        "libs/libxapi/port",
        "libs/libxapi/site",
        "build/generated",
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
    };
}

// compile_c.addBatch already adds -march=pentium3 / -target / -c / -o.
// -Xclang -fdefault-calling-conv=stdcall: the Xbox d3d8 was built /Gz (default
// __stdcall). bridge_d3d8.h is force-included to set up the NT/xtl/D3D env.
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
        "-fno-sanitize=undefined",
        "-Wno-everything",
        "-D_XAPI_",
        "-Xclang",
        "-fdefault-calling-conv=stdcall",
        "-include",
        "picolibc.h",
        "-include",
        "libs/libxapi/site/profile.h",
        "-include",
        "libs/libd3d8/site/bridge_d3d8.h",
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

    for (d3d8_sources.slices) |slice| {
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("d3d8-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("d3d8/{s}", .{slice.name}),
            .sources = slice.sources,
            .flags = cppFlags(b),
            .include_dirs = includeDirs(),
            .opt_flag = opt_flag,
            .is_cpp = slice.is_cpp,
        });
        all_outputs.appendSlice(allocator, batch.outputs) catch @panic("OOM");
        all_steps.append(allocator, batch.step) catch @panic("OOM");
    }

    const aggregate = b.allocator.create(std.Build.Step) catch @panic("OOM");
    aggregate.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "compile-d3d8-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
