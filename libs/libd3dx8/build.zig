const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const d3dx8_sources = @import("sources.zig");

const D3DX8 = "libs/libd3dx8";

// Include environment for the Xbox D3DX8 helper library. The component dirs +
// libd3dx8/inc internal deps, the public d3dx8/d3d8 headers in shared/include,
// plus libxapi's NT/xtl header set (D3DX is title-side code that links against
// the public d3d8 + CRT surface). See site/bridge_d3dx8.h, which is force-
// included before each component's pch*.h.
pub fn includeDirs() []const []const u8 {
    return &.{
        D3DX8 ++ "/inc",
        D3DX8 ++ "/core",
        D3DX8 ++ "/math",
        D3DX8 ++ "/tex",
        D3DX8 ++ "/mesh",
        D3DX8 ++ "/shape",
        D3DX8 ++ "/effect",
        D3DX8 ++ "/xof6",
        D3DX8 ++ "/misc/jpeglib",
        D3DX8 ++ "/misc/lpng105",
        D3DX8 ++ "/misc/zlib113",
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

// compile_c.addBatch already adds -march=pentium3 / -target / -c / -o.
// Unlike libd3d8 (the /Gz kernel driver), D3DX8 was built default-__cdecl
// (d3dx.mk: 386_STDCALL=0), so NO -fdefault-calling-conv=stdcall. The bridge is
// force-included to set up the NT/xtl/D3D + D3DX public-header environment and
// the MSVC-CRT / retail-build shims.
const force_includes = [_][]const u8{
    "-include",
    "picolibc.h",
    "-include",
    "libs/libxapi/site/profile.h",
    "-include",
    D3DX8 ++ "/site/bridge_d3dx8.h",
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
        "-Wno-everything",
        "-D_XAPI_",
        "-D__D3DX_INTERNAL__",
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
        "-Wno-everything",
        "-D_XAPI_",
        "-D__D3DX_INTERNAL__",
    };
    return std.mem.concat(b.allocator, []const u8, &.{ &base, &force_includes }) catch @panic("OOM");
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

    for (d3dx8_sources.slices) |slice| {
        const flags = if (slice.is_cpp) cppFlags(b) else cFlags(b);
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("d3dx8-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("d3dx8/{s}", .{slice.name}),
            .sources = slice.sources,
            .flags = flags,
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
        .name = "compile-d3dx8-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
