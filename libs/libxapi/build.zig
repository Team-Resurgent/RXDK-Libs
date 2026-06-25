const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xapi_sources = @import("sources.zig");

const XAPI = "libs/libxapi/xapi";

pub fn includeDirs() []const []const u8 {
    return &.{
        "libs/libxapi/include",
        XAPI ++ "/internal",
        XAPI ++ "/site",
        XAPI ++ "/win32",
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
        "include",
        "build/generated",
        XAPI ++ "/internal/shims",
        "include/xboxkrnl",
        "vendor/picolibc/libc/include",
        "vendor/picolibc/libc/machine/x86",
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

    const k32_extra = [_][]const u8{ "-include", "bridge_k32.h" };
    const rtl_extra = [_][]const u8{
        "-include", "bridge_rtl.h",
        "-DNTOS_KERNEL_RUNTIME=1",
        "-D_NTSYSTEM_=1",
    };
    // OHCD_XBOX_HARDWARE_ONLY / OHCD_ISOCHRONOUS_SUPPORTED are baked into the
    // ohcd sources (the Xbox build is the only configuration); USE_DMA_MACROS
    // was unreferenced. NTOS_KERNEL_RUNTIME stays (differentiates kernel slices).
    const ohcd_extra = [_][]const u8{
        "-include", "bridge_usb.h",
        "-DNTOS_KERNEL_RUNTIME=1",
    };
    // USB_ENABLE_DIRECT_CONNECT is baked into usbdev.cpp; XID_HAMMERHEAD_SUPPORT
    // was unreferenced. USB_HOST_CONTROLLER_CONFIGURATION stays (still tested).
    const usb_cpp_extra = [_][]const u8{
        "-include", "bridge_usb.h",
        "-DNTOS_KERNEL_RUNTIME=1",
        "-DUSB_HOST_CONTROLLER_CONFIGURATION=1",
    };
    const uuid_extra = [_][]const u8{
        "-include", "bridge_uuid.h",
        "-DXAPI_UUID_BUILD=1",
    };

    for (xapi_sources.slices) |slice| {
        const base_flags = if (slice.is_cpp) xapiCppFlags(b) else xapiCFlags(b);
        const extra: []const []const u8 = extra_flags: {
            if (std.mem.eql(u8, slice.name, "k32") or std.mem.eql(u8, slice.name, "dll") or std.mem.eql(u8, slice.name, "uuid")) {
                break :extra_flags if (std.mem.eql(u8, slice.name, "uuid"))
                    &uuid_extra
                else if (std.mem.eql(u8, slice.name, "k32"))
                    &k32_extra
                else if (std.mem.eql(u8, slice.name, "dll"))
                    &k32_extra
                else
                    &.{};
            }
            if (std.mem.eql(u8, slice.name, "rtl")) {
                break :extra_flags &rtl_extra;
            }
            if (std.mem.eql(u8, slice.name, "ohcd")) {
                break :extra_flags &ohcd_extra;
            }
            if (slice.is_cpp) {
                break :extra_flags &usb_cpp_extra;
            }
            break :extra_flags &.{};
        };
        const flags = xbox_target.appendFlags(b, base_flags, extra);
        const dirs = if (std.mem.eql(u8, slice.name, "uuid"))
            blk: {
                const base = includeDirs();
                var list = std.ArrayListUnmanaged([]const u8).empty;
                for (base) |dir| {
                    if (std.mem.eql(u8, dir, XAPI ++ "/win32")) {
                        list.append(b.allocator, "include/sdk") catch @panic("OOM");
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
    const install_xapi = b.addInstallFile(b.path("libs/libxapi/include/xapi.h"), "include/xapi.h");
    const install_xbox = b.addInstallFile(b.path("libs/libxapi/include/xbox.h"), "include/xbox.h");
    const install_xkbd = b.addInstallFile(b.path("libs/libxapi/include/xkbd.h"), "include/xkbd.h");
    const step = b.allocator.create(std.Build.Step) catch @panic("OOM");
    step.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "stage-libxapi-headers",
        .owner = b,
    });
    step.dependOn(&install_xapi.step);
    step.dependOn(&install_xbox.step);
    step.dependOn(&install_xkbd.step);
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
