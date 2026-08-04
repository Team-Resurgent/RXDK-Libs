const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xvoice_sources = @import("sources.zig");

const X = "libs/libxvoice";

// Include environment for the XDK-5849 voice library. The TUs include the
// PUBLIC shared/include headers (<xonline.h>/<xhv.h>/<xvoice.h>) on top of the
// force-included site/bridge_xvoice.h umbrella (compile.h NT env + xtl.h/
// xobjbase.h/dsound.h) -- same title-side shape as libxact.
pub fn includeDirs() []const []const u8 {
    return &.{
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

// Title-side default-__cdecl (the exported XHVEngine_*/XVoice* carry explicit
// WINAPI/__stdcall in the public headers), like libxact.
const common_flags = [_][]const u8{
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
    "-include",
    "picolibc.h",
    "-include",
    "libs/libxapi/site/profile.h",
    "-include",
    X ++ "/site/bridge_xvoice.h",
};

pub fn cppFlags(b: *std.Build) []const []const u8 {
    const cpp = [_][]const u8{ "-std=c++17", "-fno-exceptions", "-fno-rtti", "-nostdinc++" };
    return std.mem.concat(b.allocator, []const u8, &.{ &cpp, &common_flags }) catch @panic("OOM");
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

    for (xvoice_sources.slices) |slice| {
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("xvoice-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("xvoice/{s}", .{slice.name}),
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
        .name = "compile-xvoice-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
