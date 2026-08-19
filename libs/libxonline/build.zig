const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xonline_sources = @import("sources.zig");

const X = "libs/libxonline";

// Include environment for the Xbox Live client (private/online). Every .cpp pulls
// the precompiled header "xonp.h" (staged in inc/) which in turn includes xapip.h/
// xtl.h + the winsock/XNet surface (winsockx/winsockp/xn) + the XDK crypto
// (rsa/xcrypt/cabinet/xcabinet/dgstfile/krb5). The online-local and crypto/private
// headers not present elsewhere in RXDK live in inc/; the net headers come from
// libxnet's own tree (xonline consumes the exact header surface libxnet was built
// against); the rest reuse libxapi's NT/xtl header set + shared/include. Title-side
// client that links on top of libxnet + libkernel + libxapi + libc. See
// site/bridge_xonline.h and [[xonline-rxdk-port]].
pub fn includeDirs() []const []const u8 {
    return &.{
        X ++ "/inc",
        X ++ "/site",
        // Net headers: consume libxnet's own surface (winsockp/xn/xonlinep/xcrypt),
        // exactly what the stack below us was built against.
        "libs/libxnet/net",
        "libs/libxnet/inc",
        "shared/include",
        "libs/libxapi/internal",
        "libs/libxapi/internal/shims",
        "libs/libxapi/k32",
        "libs/libxapi/k32/inc",
        "libs/libxapi/support/inc",
        "libs/libxapi/support/inc/ntos",
        "libs/libxapi/support/crypto",
        "libs/libxapi/support/fatx",
        "libs/libxapi/nt",
        "libs/libxapi/rtl/inc",
        "libs/libxapi/port",
        "libs/libxapi/site",
        "build/generated",
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
    };
}

// Like libxnet: the online client is MSVC C++ (the CXo class, member-pointer task
// dispatch PFNXONLINE_TASK_CONTINUE = CXo::*, single-inheritance vtables, struct
// layouts). Compile with clang's MSVC C++ ABI so member-ptr/vtable/struct layouts
// are internally consistent and match the winsock/XNet C ABI it calls into.
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
    // -fdefault-calling-conv=stdcall: the online client was built /Gz, like the net
    // stack below it; this makes its vendor kernel/net calls compile __stdcall so
    // they bind straight to libkernel.lib / libxnet.lib (no cdecl->stdcall facades).
    "-Xclang",
    "-fdefault-calling-conv=stdcall",
    "-include",
    X ++ "/site/cdecl_libc.h",
    "-include",
    "picolibc.h",
    "-include",
    "libs/libxapi/site/profile.h",
    "-include",
    X ++ "/site/bridge_xonline.h",
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
    _ = xbox_target; // libxonline pins its own MSVC target_triple (see note above)
    const allocator = b.allocator;
    var all_outputs = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    var all_steps = std.ArrayListUnmanaged(*std.Build.Step).empty;

    for (xonline_sources.slices) |slice| {
        const flags = if (slice.is_cpp) cppFlags(b) else cFlags(b);
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("xonline-{s}", .{slice.name}),
            .target = target_triple, // MSVC C++ ABI (see note above), not xbox_target's gnu
            .out_subdir = b.fmt("xonline/{s}", .{slice.name}),
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
        .name = "compile-xonline-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}

// The UIX drop-in UI, compiled in exactly the same environment as the rest of
// libxonline but packed into its own archive -- 5849 ships it as uix.lib, not
// inside xonline.lib. It calls into libxonline, so a title links both.
pub fn addUixObjects(
    b: *std.Build,
    xbox_target: @TypeOf(@import("../../build/xbox_target.zig")),
    opt_flag: []const u8,
) ObjectBatch {
    _ = xbox_target; // pins its own MSVC target_triple, as above

    const batch = compile_c.addBatch(b, .{
        .name = "uix",
        .target = target_triple,
        .out_subdir = "uix",
        .sources = &xonline_sources.uix_sources,
        .flags = cppFlags(b),
        .include_dirs = includeDirs(),
        .opt_flag = opt_flag,
        .is_cpp = true,
    });

    return .{ .step = batch.step, .outputs = batch.outputs };
}
