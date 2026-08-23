const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xapi_sources = @import("sources.zig");

const XAPI = "libs/libxapi";

pub fn includeDirs() []const []const u8 {
    return &.{
        "shared/include",
        XAPI ++ "/internal",
        XAPI ++ "/site",
        XAPI ++ "/nt",
        XAPI ++ "/k32",
        XAPI ++ "/k32/inc",
        XAPI ++ "/dll",
        XAPI ++ "/rtl/inc",
        XAPI ++ "/rtl",
        XAPI ++ "/uuid",
        XAPI ++ "/usb/inc",
        XAPI ++ "/usb/ohcd",
        XAPI ++ "/usb/usbd",
        XAPI ++ "/usb/hub",
        XAPI ++ "/usb/mu",
        XAPI ++ "/usb/xid",
        XAPI ++ "/support/inc",
        XAPI ++ "/support/inc/ntos",
        XAPI ++ "/support/fatx",
        XAPI ++ "/support/idex",
        XAPI ++ "/support/crypto",
        XAPI ++ "/port",
        "build/generated",
        XAPI ++ "/internal/shims",
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
        // ms_printf.h: the MSVC->C99 format translator, shared with the wide
        // Win32 formatters in k32 (wsprintfW and friends promise MSVC's
        // spelling of %s/%S/%I64, which picolibc does not implement).
        "libs/libc/xbox",
    };
}

pub fn xapiCFlags(_: *std.Build) []const []const u8 {
    return &.{
        "-std=c17",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fno-zero-initialized-in-bss",
        "-fdata-sections",
        "-ffunction-sections",
        "-fno-sanitize=undefined",
        "-fno-builtin",
        "-Wno-everything",
        "-nostdinc",
        "-fms-extensions",
        "-fms-compatibility",
        "-fno-emulated-tls",
        "-include",
        "picolibc.h",
        "-include",
        XAPI ++ "/site/profile.h",
        "-D_XAPI_",
    };
}

pub fn xapiCppFlags(_: *std.Build) []const []const u8 {
    return &.{
        "-std=c++17",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fdata-sections",
        "-ffunction-sections",
        "-fno-exceptions",
        "-frtti",
        "-nostdinc",
        "-nostdinc++",
        "-fno-sanitize=undefined",
        "-fno-builtin",
        "-Wno-everything",
        "-fms-extensions",
        "-fms-compatibility",
        "-include",
        "picolibc.h",
        "-include",
        XAPI ++ "/site/profile.h",
        "-D_XAPI_",
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

    for (xapi_sources.slices) |slice| {
        // Bridges are #included by each source file now, so the build passes no
        // per-slice -include or -D — only the base C/C++ flags differ by
        // language, plus uuid's distinct SDK include path.
        const flags = if (slice.is_cpp) xapiCppFlags(b) else xapiCFlags(b);
        const dirs = if (std.mem.eql(u8, slice.name, "uuid"))
            blk: {
                const base = includeDirs();
                var list = std.ArrayListUnmanaged([]const u8).empty;
                for (base) |dir| {
                    // uuid compiles against the fuller XDK SDK headers it needs (the
                    // COM/OLE + rpc + win* set), vendored self-contained under
                    // uuid/sdkinc (the exact transitive closure copied out of the old
                    // May-2020-leak submodule, which is no longer a dependency). Placed
                    // ahead of shared/include so their windef/winbase shadow the
                    // slimmed shared copies.
                    if (std.mem.eql(u8, dir, "shared/include")) {
                        // site/uuid_shim goes first: it shadows winnt.h with a copy whose
                        // Int64Sh*Mod32 definitions are static, so they stop being emitted
                        // with external linkage in every one of this slice's objects. See
                        // the note at the top of that file.
                        list.append(b.allocator, XAPI ++ "/site/uuid_shim") catch @panic("OOM");
                        list.append(b.allocator, XAPI ++ "/uuid/sdkinc") catch @panic("OOM");
                    }
                    list.append(b.allocator, dir) catch @panic("OOM");
                }
                break :blk list.toOwnedSlice(b.allocator) catch @panic("OOM");
            }
        else
            includeDirs();
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("xapi-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("xapi/{s}", .{slice.name}),
            .sources = slice.sources,
            .flags = flags,
            .include_dirs = dirs,
            .opt_flag = opt_flag,
            .is_cpp = slice.is_cpp,
        });
        all_outputs.appendSlice(allocator, batch.outputs) catch @panic("OOM");
        all_steps.append(allocator, batch.step) catch @panic("OOM");
    }

    const aggregate = b.allocator.create(std.Build.Step) catch @panic("OOM");
    aggregate.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "compile-xapi-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}

pub fn stageHeaders(b: *std.Build) *std.Build.Step {
    const step = b.allocator.create(std.Build.Step) catch @panic("OOM");
    step.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "stage-libxapi-headers",
        .owner = b,
    });
    // libxapi's public headers + the Win32 base they need, from shared/include.
    // The master umbrella (<xtl.h>) is not a libxapi header -- it ships to the
    // distribution from shared/include directly (see build-iso.ps1). The
    // xboxkrnl/ and xbox/ subdirs are staged by the libc header step.
    const public_headers = [_][]const u8{
        "xapi.h", "xbox.h", "xkbd.h",
        "windef.h", "winbase.h", "winerror.h",
    };
    for (public_headers) |name| {
        const install = b.addInstallFile(
            b.path(b.fmt("shared/include/{s}", .{name})),
            b.fmt("include/{s}", .{name}),
        );
        step.dependOn(&install.step);
    }
    return step;
}