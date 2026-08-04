const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xact_sources = @import("sources.zig");

const X = "libs/libxact";

// Include environment for the Xbox XACT runtime engine. The engine/*.cpp pull
// "xacti.h" (-> "common.h" + <wavbndlr.h> + "xactp.h") and "xboxdbg.h";
// site/bridge_xact.h (force-included) pre-pulls the xtl/dsound/xboxkrnl umbrella.
// Title-side, like libxmv -- reuses libxapi's NT/xtl header set + shared/include
// public headers (dsound.h + the DirectSound types xactp.h names).
pub fn includeDirs() []const []const u8 {
    return &.{
        X ++ "/inc",
        X ++ "/site",
        X ++ "/wma",
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

// -fasm-blocks: drvhlp.h keeps its MSVC __asm{} interlocked and/or helpers verbatim
// (clang compiles them natively, like libdsound). -fno-operator-names: drvhlp.h has
// functions literally named `and`/`or` (C++ alternative tokens). -DDPF_LIBRARY:
// debug.h requires it (the engine tags debug output). The bridge supplies the
// Win32/COM/CRT env. Title-side default-__cdecl (exports carry explicit __stdcall).
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
    "-fno-builtin",
    "-Wno-everything",
    "-D_XAPI_",
    "-DDPF_LIBRARY=\"XACTENG\"",
    "-DNAMESPACE=XACT",
    "-DDPFLVL_DEFAULT=4",
    "-include",
    "picolibc.h",
    "-include",
    "libs/libxapi/site/profile.h",
    "-include",
    X ++ "/site/bridge_xact.h",
};

pub fn cppFlags(b: *std.Build) []const []const u8 {
    const cpp = [_][]const u8{ "-std=c++17", "-fno-exceptions", "-fno-rtti", "-nostdinc++" };
    return std.mem.concat(b.allocator, []const u8, &.{ &cpp, &common_flags }) catch @panic("OOM");
}

pub fn cFlags(b: *std.Build) []const []const u8 {
    const c = [_][]const u8{"-std=c17"};
    return std.mem.concat(b.allocator, []const u8, &.{ &c, &common_flags }) catch @panic("OOM");
}

// Minimal C environment for the ported ffmpeg WMA decoder: picolibc + freestanding only, WITHOUT
// the engine's bridge_xact.h/profile.h/DPF force-includes and MS-extension flags (the decoder is
// plain C99 and its shim would clash with the XACT/COM umbrella).
const wma_common_flags = [_][]const u8{
    "-ffreestanding",
    "-fno-stack-protector",
    "-fdata-sections",
    "-ffunction-sections",
    "-nostdinc",
    "-fno-sanitize=undefined",
    "-fno-builtin",
    "-Wno-everything",
    "-include",
    "picolibc.h",
};

pub fn wmaFlags(b: *std.Build) []const []const u8 {
    const c = [_][]const u8{"-std=c99"};
    return std.mem.concat(b.allocator, []const u8, &.{ &c, &wma_common_flags }) catch @panic("OOM");
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

    for (xact_sources.slices) |slice| {
        const flags = if (slice.is_cpp) cppFlags(b) else if (slice.minimal_c) wmaFlags(b) else cFlags(b);
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("xact-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("xact/{s}", .{slice.name}),
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
        .name = "compile-xact-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
