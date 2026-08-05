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
    "-fno-builtin",
    // RXDK: MSVC-ABI clang emits __chkstk (allocate+probe) for >4KB stack frames, but the GNU
    // compiler-rt only supplies __chkstk_ms. Xbox stacks are fully committed (no guard pages to
    // probe), so disable the probe -- the compiler does a direct ESP subtraction instead.
    "-mno-stack-arg-probe",
    "-fwrapv",
    "-Wno-everything",
    "-D_XAPI_",
    // -fdefault-calling-conv=stdcall: the Xbox net stack was /Gz; this makes the
    // vendor kernel calls compile __stdcall so they bind straight to libkernel.lib
    // (no cdecl->stdcall facades). cdecl_libc.h is force-included FIRST to pin
    // libc to __cdecl before picolibc's headers are seen (see that file).
    "-Xclang",
    "-fdefault-calling-conv=stdcall",
    "-include",
    X ++ "/site/cdecl_libc.h",
    "-include",
    "picolibc.h",
    "-include",
    "libs/libxapi/site/profile.h",
    "-include",
    X ++ "/site/bridge_xnet.h",
};

// The two library variants the retail XDK ships. LIBX is the plain sockets stack
// (libxnet.lib); LIBO adds ONLINE/QOS/SG (libxneto.lib) and, through the SG
// Kerberos key exchange in ip.cpp, requires libxonline to link. See
// site/bridge_xnet.h. Absent a define, that header defaults to LIBO.
fn variantFlags(b: *std.Build, libo: bool) []const []const u8 {
    const v = [_][]const u8{if (libo) "-DXNET_BUILD_LIBO=1" else "-DXNET_BUILD_LIBX=1"};
    return std.mem.concat(b.allocator, []const u8, &.{ &v, &common_flags }) catch @panic("OOM");
}

pub fn cppFlagsFor(b: *std.Build, libo: bool) []const []const u8 {
    const cpp = [_][]const u8{ "-std=c++17", "-fno-exceptions", "-fno-rtti", "-nostdinc++" };
    return std.mem.concat(b.allocator, []const u8, &.{ &cpp, variantFlags(b, libo) }) catch @panic("OOM");
}

pub fn cFlagsFor(b: *std.Build, libo: bool) []const []const u8 {
    const c = [_][]const u8{"-std=c17"};
    return std.mem.concat(b.allocator, []const u8, &.{ &c, variantFlags(b, libo) }) catch @panic("OOM");
}

pub fn cppFlags(b: *std.Build) []const []const u8 {
    return cppFlagsFor(b, false);
}

pub fn cFlags(b: *std.Build) []const []const u8 {
    return cFlagsFor(b, false);
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
    libo: bool,
) ObjectBatch {
    _ = xbox_target; // libxnet pins its own MSVC target_triple (see note above)
    const allocator = b.allocator;
    const tag = if (libo) "xneto" else "xnet";
    var all_outputs = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    var all_steps = std.ArrayListUnmanaged(*std.Build.Step).empty;

    for (xnet_sources.slices) |slice| {
        const flags = if (slice.is_cpp) cppFlagsFor(b, libo) else cFlagsFor(b, libo);
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("{s}-{s}", .{ tag, slice.name }),
            .target = target_triple,   // MSVC C++ ABI (see note above), not xbox_target's gnu
            .out_subdir = b.fmt("{s}/{s}", .{ tag, slice.name }),
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
        .name = b.fmt("compile-{s}-all", .{tag}),
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
