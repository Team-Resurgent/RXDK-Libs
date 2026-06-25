const std = @import("std");
const compile_c = @import("compile_c.zig");
const xapi_sources = @import("xapi_sources.zig");

const VENDOR = "vendor/xbox_private";

pub fn includeDirs() []const []const u8 {
    return &.{
        "include",
        "shared/include",
        "include/xapi",
        "include/xdk",
        "include/sdk",
        "build/generated",
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
        VENDOR ++ "/private/ntos/xapi/k32",
        VENDOR ++ "/private/ntos/xapi/inc",
        VENDOR ++ "/private/ntos/xapi/dll",
        VENDOR ++ "/private/ntos/rtl/inc",
        VENDOR ++ "/private/ntos/rtl",
        VENDOR ++ "/private/ntos/inc",
        VENDOR ++ "/private/ntos/fatx",
        VENDOR ++ "/private/ntos/idex",
        VENDOR ++ "/private/inc",
        VENDOR ++ "/private/inc/crypto",
        VENDOR ++ "/private/ntos/dd/usb/inc",
        VENDOR ++ "/private/ntos/dd/usb/ohcd",
        VENDOR ++ "/private/ntos/dd/usb/usbd",
        VENDOR ++ "/private/ntos/dd/usb/usbhub",
        VENDOR ++ "/private/ntos/dd/usb/mu",
        VENDOR ++ "/private/ntos/dd/usb/xid",
        VENDOR ++ "/private/ntos/dd/usb/xmouse_dbg",
        VENDOR ++ "/private/ntos/dd/usb/xidex",
        VENDOR ++ "/private/genx/types/uuid",
    };
}

pub const ObjectBatch = struct {
    step: *std.Build.Step,
    outputs: []const std.Build.LazyPath,
};

pub fn addAllObjects(
    b: *std.Build,
    xbox_target: @TypeOf(@import("xbox_target.zig")),
    opt_flag: []const u8,
) ObjectBatch {
    const allocator = b.allocator;
    var all_outputs = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    var all_steps = std.ArrayListUnmanaged(*std.Build.Step).empty;

    const k32_extra = [_][]const u8{ "-include", "k32_bridge.h" };
    const rtl_extra = [_][]const u8{
        "-include", "rtl_bridge.h",
        "-DNTOS_KERNEL_RUNTIME=1",
        "-D_NTSYSTEM_=1",
    };
    const ohcd_extra = [_][]const u8{
        "-include", "usb_bridge.h",
        "-DNTOS_KERNEL_RUNTIME=1",
        "-DUSE_DMA_MACROS",
        "-DOHCD_XBOX_HARDWARE_ONLY",
        "-DOHCD_ISOCHRONOUS_SUPPORTED",
    };
    const usb_cpp_extra = [_][]const u8{
        "-include", "usb_bridge.h",
        "-DNTOS_KERNEL_RUNTIME=1",
        "-DUSB_HOST_CONTROLLER_CONFIGURATION=1",
        "-DUSB_ENABLE_DIRECT_CONNECT",
        "-DXID_HAMMERHEAD_SUPPORT",
    };

    for (xapi_sources.slices) |slice| {
        const base_flags = if (slice.is_cpp) xbox_target.xapiCppFlags(b) else xbox_target.xapiCFlags(b);
        const extra: []const []const u8 = extra_flags: {
            if (std.mem.eql(u8, slice.name, "k32") or std.mem.eql(u8, slice.name, "dll") or std.mem.eql(u8, slice.name, "uuid")) {
                break :extra_flags if (std.mem.eql(u8, slice.name, "uuid"))
                    &[_][]const u8{"-DXAPI_UUID_BUILD=1"}
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
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("xapi-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("xapi/{s}", .{slice.name}),
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
    const xdk = b.addInstallDirectory(.{
        .source_dir = b.path("include/xdk"),
        .install_dir = .prefix,
        .install_subdir = "include/xdk",
    });
    const xapi_hdr = b.addInstallDirectory(.{
        .source_dir = b.path("include/xapi"),
        .install_dir = .prefix,
        .install_subdir = "include/xapi",
    });
    const site = b.addInstallFile(b.path("build/generated/xapi_site.h"), "include/xapi_site.h");
    const step = b.allocator.create(std.Build.Step) catch @panic("OOM");
    step.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "stage-xapi-headers",
        .owner = b,
    });
    step.dependOn(&xdk.step);
    step.dependOn(&xapi_hdr.step);
    step.dependOn(&site.step);
    return step;
}
