const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const ds_sources = @import("sources.zig");

const DS = "libs/libdsound";

// Include environment for the Xbox core DirectSound library. Component dirs +
// the public dsound/dsoundp/dsfxparm headers in shared/include, plus libxapi's
// NT/xtl header set (dsound is a kernel+title hybrid -- the MCPX APU driver).
// See site/bridge_dsound.h (force-included before each TU's precomp).
pub fn includeDirs() []const []const u8 {
    return &.{
        DS ++ "/dsound",
        DS ++ "/common",
        DS ++ "/ac97",
        DS ++ "/tools_inc",
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

// Shared flags. -fasm-blocks: keep the few hand-written MSVC __asm{} blocks
// (dsmath/i3dl2/drvhlp) verbatim. -fno-operator-names: drvhlp.h has functions
// literally named `and`/`or` (C++ alternative tokens). -DDPF_LIBRARY: the build
// tags debug-print output (dsound/sources). The bridge supplies the rest.
// -Xclang -fdefault-calling-conv=stdcall: the Xbox DirectSound was built /Gz
// (default __stdcall), like libd3d8. This makes the vendor kernel calls compile
// __stdcall so they bind straight to libkernel.lib's __imp__Name@N -- no
// cdecl->stdcall facades. cdecl_libc.h is force-included FIRST to pin libc/libm
// to __cdecl before picolibc's headers are seen (see that file).
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
    "-DDPF_LIBRARY=\"DSOUND\"",
    "-Xclang",
    "-fdefault-calling-conv=stdcall",
    "-include",
    DS ++ "/site/cdecl_libc.h",
    "-include",
    "picolibc.h",
    "-include",
    "libs/libxapi/site/profile.h",
    "-include",
    DS ++ "/site/bridge_dsound.h",
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

    for (ds_sources.slices) |slice| {
        const flags = if (slice.is_cpp) cppFlags(b) else cFlags(b);
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("dsound-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("dsound/{s}", .{slice.name}),
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
        .name = "compile-dsound-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
