const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");

const picolibc_exclude = [_][]const u8{
    "conv_flt.c",
    "ultoa_invert.c",
    "vfprintf_char.c",
    "vfprintf_float.c",
    "vfprintf_int.c",
    "vfprintf_n.c",
    "vfprintf_str.c",
    "sf_gamma.c",
    "s_gamma.c",
    "sf_exp.c",
    "sf_exp2.c",
    "sf_log.c",
    "sf_log2.c",
    "sf_pow.c",
    "s_exp.c",
    "s_exp2.c",
    "s_log.c",
    "s_log2.c",
    "s_pow.c",
    "sinf.c",
    "sincosf.c",
    "sincosf_data.c",
    "s_scalbn.c",
    "cosf.c",
    "ftoa_ryu.c",
    "dtoa_ryu.c",
    "atod_ryu.c",
    "atof_ryu.c",
    "ryu_divpow2.c",
    "ryu_log10.c",
    "ryu_log2pow5.c",
    "ryu_pow5bits.c",
    "ryu_table.c",
    "ryu_umul128.c",
    "tls.c",
    "interrupt.c",
    "posix_locale.c",
    "posixiob_stdin.c",
    "posixiob_stdout.c",
    "posixiob_stderr.c",
    "tcb-32.S",
    "tcb-64.S",
    "tcb.S",
};

const picolibc_subdirs = [_][]const u8{
    "libc/ctype",
    "libc/errno",
    "libc/misc",
    "libc/picolib",
    "libc/search",
    "libc/stdlib",
    "libc/stdio",
    "libc/string",
    "libc/time",
    "libc/locale",
};

const libm_sources = [_][]const u8{
    "vendor/picolibc/libm/common/s_scalbn.c",
    "vendor/picolibc/libm/common/sf_scalbn.c",
    "vendor/picolibc/libm/common/s_fpclassify.c",
    "vendor/picolibc/libm/common/sf_fpclassify.c",
    "vendor/picolibc/libm/common/s_finite.c",
    "vendor/picolibc/libm/common/sf_finite.c",
    "vendor/picolibc/libm/common/s_isnan.c",
    "vendor/picolibc/libm/common/sf_isnan.c",
    "vendor/picolibc/libm/common/s_isinf.c",
    "vendor/picolibc/libm/common/sf_isinf.c",
    "vendor/picolibc/libm/common/math_err_check_uflow.c",
    "vendor/picolibc/libm/common/math_err_check_oflow.c",
    "vendor/picolibc/libm/common/math_err_uflow.c",
    "vendor/picolibc/libm/common/math_err_oflow.c",
    "vendor/picolibc/libm/common/math_err_with_errno.c",
    "vendor/picolibc/libm/common/math_errf_check_uflowf.c",
    "vendor/picolibc/libm/common/math_errf_uflowf.c",
    "vendor/picolibc/libm/common/math_errf_oflowf.c",
    "vendor/picolibc/libm/common/math_errf_with_errnof.c",
    "vendor/picolibc/libm/ld/math_errl_check_uflowl.c",
    "vendor/picolibc/libm/ld/math_errl_uflowl.c",
    "vendor/picolibc/libm/ld/math_errl_oflowl.c",
    "vendor/picolibc/libm/ld/math_errl_with_errnol.c",
    "vendor/picolibc/libm/ld/s_frexpl.c",
    "vendor/picolibc/libm/ld/s_scalbnl.c",
    "vendor/picolibc/libm/ld/s_fpclassifyl.c",
    "vendor/picolibc/libm/ld/s_finitel.c",
    "vendor/picolibc/libm/ld/s_isnanl.c",
    "vendor/picolibc/libm/ld/s_isinfl.c",
};

pub fn collectSources(b: *std.Build, allocator: std.mem.Allocator) ![]const []const u8 {
    var list = std.ArrayListUnmanaged([]const u8).empty;
    errdefer list.deinit(allocator);

    for (picolibc_subdirs) |sub| {
        try appendDirSources(b, allocator, &list, sub, ".c", &picolibc_exclude);
    }
    for (libm_sources) |src| {
        try list.append(allocator, src);
    }
    try list.append(allocator, "libs/libc/xbox/posix_stdio_streams.c");

    return try list.toOwnedSlice(allocator);
}

fn appendDirSources(
    b: *std.Build,
    allocator: std.mem.Allocator,
    list: *std.ArrayListUnmanaged([]const u8),
    sub: []const u8,
    ext: []const u8,
    exclude_list: []const []const u8,
) !void {
    const io = b.graph.io;
    var dir = b.build_root.handle.openDir(io, b.fmt("vendor/picolibc/{s}", .{sub}), .{ .iterate = true }) catch return;
    defer dir.close(io);

    var it = dir.iterate();
    dir_loop: while (try it.next(io)) |entry| {
        if (entry.kind != .file) continue;
        if (!std.mem.endsWith(u8, entry.name, ext)) continue;
        for (exclude_list) |skip| {
            if (std.mem.eql(u8, entry.name, skip)) continue :dir_loop;
        }
        for (picolibc_exclude) |skip| {
            if (std.mem.eql(u8, entry.name, skip)) continue :dir_loop;
        }
        const rel = try std.fmt.allocPrint(allocator, "vendor/picolibc/{s}/{s}", .{ sub, entry.name });
        try list.append(allocator, rel);
    }
}

fn includeDirs(_: *std.Build) []const []const u8 {
    return &.{
        "shared/include",
        "build/generated",
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
        "vendor/picolibc/libm/common",
        "vendor/picolibc/libm/ld",
        "vendor/picolibc/libc/locale",
        "vendor/picolibc/libc/ctype",
        "vendor/picolibc/libc/stdio",
    };
}

pub fn addPicolibcObjects(
    b: *std.Build,
    xbox_target: @TypeOf(@import("../../build/xbox_target.zig")),
    opt_flag: []const u8,
) !compile_c.CompileBatch {
    const sources = try collectSources(b, b.allocator);
    return compile_c.addBatch(b, .{
        .name = "picolibc",
        .target = xbox_target.target_triple,
        .out_subdir = "picolibc",
        .sources = sources,
        .flags = xbox_target.picolibcFlags(b),
        .include_dirs = includeDirs(b),
        .opt_flag = opt_flag,
    });
}

pub fn addXboxObjects(
    b: *std.Build,
    xbox_target: @TypeOf(@import("../../build/xbox_target.zig")),
    opt_flag: []const u8,
) compile_c.CompileBatch {
    const sources = [_][]const u8{
        "libs/libc/xbox/hal.c",
        "libs/libc/xbox/fileio.c",
        "libs/libc/xbox/startup.c",
        "libs/libc/xbox/trace.c",
        "libs/libc/xbox/stubs.c",
        "libs/libc/xbox/tls_stub.c",
        "libs/libc/xbox/picolibc_aliases.c",
        "libs/libc/c23/stdbit.c",
        "libs/libc/xbox/crt0.S",
    };
    return compile_c.addBatch(b, .{
        .name = "xbox",
        .target = xbox_target.target_triple,
        .out_subdir = "xbox",
        .sources = &sources,
        .flags = xbox_target.cFlags(b),
        .include_dirs = includeDirs(b),
        .opt_flag = opt_flag,
    });
}

pub fn stageHeaders(b: *std.Build) *std.Build.Step {
    const install = b.addInstallDirectory(.{
        .source_dir = b.path("shared/picolibc/include"),
        .install_dir = .prefix,
        .install_subdir = "include",
    });
    const gen = b.addInstallFile(b.path("build/generated/picolibc.h"), "include/picolibc.h");
    const xbox = b.addInstallDirectory(.{
        .source_dir = b.path("shared/include/xbox"),
        .install_dir = .prefix,
        .install_subdir = "include/xbox",
    });
    const xboxkrnl = b.addInstallDirectory(.{
        .source_dir = b.path("shared/include/xboxkrnl"),
        .install_dir = .prefix,
        .install_subdir = "include/xboxkrnl",
    });
    const c23 = b.addInstallDirectory(.{
        .source_dir = b.path("libs/libc/c23"),
        .install_dir = .prefix,
        .install_subdir = "include",
    });

    const step = b.allocator.create(std.Build.Step) catch @panic("OOM");
    step.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "stage-picolibc-headers",
        .owner = b,
    });
    step.dependOn(&install.step);
    step.dependOn(&gen.step);
    step.dependOn(&xbox.step);
    step.dependOn(&xboxkrnl.step);
    step.dependOn(&c23.step);
    return step;
}
