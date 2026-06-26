const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xapi_sources = @import("sources.zig");

const XAPI = "libs/libxapi/xapi";

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
        XAPI ++ "/minilib",
        "build/generated",
        XAPI ++ "/internal/shims",
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
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
                    // uuid compiles against the full XDK SDK headers from the
                    // vendored May-2020 leak; place them ahead of shared/include
                    // so their windef/winbase shadow the slimmed shared copies.
                    if (std.mem.eql(u8, dir, "shared/include")) {
                        list.append(b.allocator, "vendor/xbox_leak_may_2020/xbox_leak_may_2020/xbox trunk/xbox/public/sdk/inc") catch @panic("OOM");
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
    // Public umbrella set in shared/include (xt.h pulls windef/winbase; the
    // xboxkrnl/ and xbox/ subdirs are staged by the libc header step).
    const public_headers = [_][]const u8{
        "xt.h", "xapi.h", "xbox.h", "xkbd.h",
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

pub fn isCoreObjectPath(path: []const u8) bool {
    const prefixes = [_][]const u8{
        "libs/libxapi/xapi/k32/",
        "libs\\libxapi\\xapi\\k32\\",
        "libs/libxapi/xapi/dll/",
        "libs\\libxapi\\xapi\\dll\\",
        "libs/libxapi/xapi/rtl/",
        "libs\\libxapi\\xapi\\rtl\\",
        "libs/libxapi/xapi/uuid/",
        "libs\\libxapi\\xapi\\uuid\\",
        "libs/libxapi/xapi/port/",
        "libs\\libxapi\\xapi\\port\\",
        "libs/libxapi/xapi/minilib/",
        "libs\\libxapi\\xapi\\minilib\\",
    };
    for (prefixes) |prefix| {
        if (std.mem.indexOf(u8, path, prefix) != null) {
            return true;
        }
    }
    return false;
}
