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
// The XDK shipped two builds of this library: retail d3d8.lib, and d3d8i.lib with
// the performance instrumentation compiled in. That split is in the source -- the
// counters and the whole D3DPERF statistics surface in se/stats.cpp sit behind
// `#if PROFILE`, which is off unless DBG. So the instrumented variant is not a
// fork, it is the same sources with PROFILE defined.
//
// Retail exports only the D3DPERF event markers (BeginEvent/EndEvent/SetMarker);
// confirmed against the 5849 libs, where d3d8.lib carries just those and
// d3d8i.lib/d3d8d.lib carry GetStatistics/Reset/GetPushBufferInfo/Start+
// StopPerfProfile as well. Keeping that split means a retail title cannot
// accidentally link a counter that would always read zero.
pub fn cppFlagsFor(b: *std.Build, profile: bool) []const []const u8 {
    const base = cppFlags(b);
    if (!profile) return base;

    var list = std.ArrayListUnmanaged([]const u8).empty;
    list.appendSlice(b.allocator, base) catch @panic("OOM");
    list.append(b.allocator, "-DPROFILE=1") catch @panic("OOM");
    return list.toOwnedSlice(b.allocator) catch @panic("OOM");
}

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
        "-fno-builtin",
        "-Wno-everything",
        "-D_XAPI_",
        "-Xclang",
        "-fdefault-calling-conv=stdcall",
        // FIRST force-include: pins libc/libm calls to cdecl before any system
        // header declares them, so -fdefault-calling-conv=stdcall + -ffreestanding
        // doesn't silently make them stdcall (see cdecl_libc.h).
        "-include",
        "libs/libd3d8/site/cdecl_libc.h",
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
    return addAllObjectsVariant(b, xbox_target, opt_flag, false);
}

pub fn addAllObjectsVariant(
    b: *std.Build,
    xbox_target: @TypeOf(@import("../../build/xbox_target.zig")),
    opt_flag: []const u8,
    profile: bool,
) ObjectBatch {
    const allocator = b.allocator;
    var all_outputs = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    var all_steps = std.ArrayListUnmanaged(*std.Build.Step).empty;

    // mpintr.cpp (the NV2A interrupt module) documents itself "optimize this
    // module for speed, not for size" -- the original MSVC #pragma optimize("t").
    // That intent was lost in the port, so under ReleaseSmall (-Os) Clang's size
    // optimizations miscompile its SoftwareMethod handler and the GPU scans out
    // garbage from the second frame on (visible corruption; xemu pfifo_run_pusher
    // assert). -O2 and -O0 are both correct. So honor the original intent: build
    // mpintr for speed (-O2) in any release mode; Debug stays -O0.
    const mpintr_opt: []const u8 = if (std.mem.eql(u8, opt_flag, "-O0")) opt_flag else "-O2";

    for (d3d8_sources.slices) |slice| {
        for (slice.sources) |src| {
            const use_opt = if (std.mem.indexOf(u8, src, "mpintr") != null) mpintr_opt else opt_flag;
            const one = allocator.dupe([]const u8, &.{src}) catch @panic("OOM");
            const batch = compile_c.addBatch(b, .{
                .name = if (profile) b.fmt("d3d8i-{s}", .{std.fs.path.stem(src)}) else b.fmt("d3d8-{s}", .{std.fs.path.stem(src)}),
                .target = xbox_target.target_triple,
                .out_subdir = if (profile) b.fmt("d3d8i/{s}", .{slice.name}) else b.fmt("d3d8/{s}", .{slice.name}),
                .sources = one,
                .flags = cppFlagsFor(b, profile),
                .include_dirs = includeDirs(),
                .opt_flag = use_opt,
                .is_cpp = slice.is_cpp,
            });
            all_outputs.appendSlice(allocator, batch.outputs) catch @panic("OOM");
            all_steps.append(allocator, batch.step) catch @panic("OOM");
        }
    }

    const aggregate = b.allocator.create(std.Build.Step) catch @panic("OOM");
    aggregate.* = std.Build.Step.init(.{
        .id = .custom,
        .name = if (profile) "compile-d3d8i-all" else "compile-d3d8-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
