const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xnet_sources = @import("sources.zig");

const X = "libs/libxnet";

// Include environment for the Xbox net stack (private/ntos/net). xnp.h (the
// precomp, #included by every .cpp) pulls ntos/init/hal/nturtl/xtl/xboxp/xdbg/phy/
// xbeimage/ldr + winsockp/xcrypt/xonlinep; most live in libxapi's header tree, the
// rest are copied into inc/. Kernel-runtime component -- see site/bridge_xnet.h.
pub fn includeDirs() []const []const u8 {
    return &.{
        X ++ "/net",
        X ++ "/inc",
        X ++ "/lib",
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

// The newer net stack is MSVC C++ (member-ptr-to-base casts, single-inheritance
// vtables, struct layouts asserted via C_ASSERT). Compile with clang's MSVC C++
// ABI so layouts + the native CXbdmServer/CXbdmClient interop match the kit's
// MSVC-built xbdm. C ABI (the extern "C"/WSAAPI entry points) is unchanged, so the
// objects link fine with the gnu-ABI rest of the tree.
pub const target_triple = "x86-windows-msvc";

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
    "-fwrapv",
    "-Wno-everything",
    "-D_XAPI_",
    "-include",
    "picolibc.h",
    "-include",
    "libs/libxapi/site/profile.h",
    "-include",
    X ++ "/site/bridge_xnet.h",
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
    _ = xbox_target; // libxnet pins its own MSVC target_triple (see note above)
    const allocator = b.allocator;
    var all_outputs = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    var all_steps = std.ArrayListUnmanaged(*std.Build.Step).empty;

    for (xnet_sources.slices) |slice| {
        const flags = if (slice.is_cpp) cppFlags(b) else cFlags(b);
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("xnet-{s}", .{slice.name}),
            .target = target_triple,   // MSVC C++ ABI (see note above), not xbox_target's gnu
            .out_subdir = b.fmt("xnet/{s}", .{slice.name}),
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
        .name = "compile-xnet-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
