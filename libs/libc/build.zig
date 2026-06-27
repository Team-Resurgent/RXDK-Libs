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
    "clock.c", // replaced by libs/libc/xbox/timeio.c (KeQueryPerformanceCounter)
    "lock.c",  // replaced by libs/libc/xbox/locks.c (RTL critical sections)
    "remove.c", // picolibc's is unlink-only; dirio.c provides a POSIX remove (rmdir for dirs)
    "tmpnam.c", // picolibc's ignore P_tmpdir; tmpio.c targets the Z: scratch drive
    "tmpfile.c",
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

// libm/common and libm/math are globbed (full double + float math). Only the
// long-double (ld) pieces we want are listed explicitly, since ld is not
// globbed wholesale (80-bit x87 long double support is partial).
const libm_ld_sources = [_][]const u8{
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
    // Full double + float math, complex, and fenv; long double stays selective.
    try appendLibmDir(b, allocator, &list, "libm/common", &libm_common_exclude, true, false);
    try appendLibmDir(b, allocator, &list, "libm/math", &libm_math_exclude, true, false);
    try appendLibmDir(b, allocator, &list, "libm/complex", &libm_complex_exclude, false, false);
    // x86 fenv: use the x87/SSE implementation (libm/machine/x86/fenv.c) and
    // drop the generic soft-float fenv.c. The other helpers (fe_dfl_env,
    // fegetmode, fesetmode) are arch-generic and stay.
    try appendLibmDir(b, allocator, &list, "libm/fenv", &libm_fenv_exclude, false, false);
    try list.append(allocator, "vendor/picolibc/libm/machine/x86/fenv.c");
    for (libm_ld_sources) |src| {
        try list.append(allocator, src);
    }
    try list.append(allocator, "libs/libc/xbox/posix_stdio_streams.c");
    // x86 setjmp/longjmp (machine asm; needs <picolibc.h> + relative i386mach.h)
    try list.append(allocator, "vendor/picolibc/libc/machine/x86/setjmp.S");
    // POSIX regex (Henry Spencer engine; internal headers are same-dir quote includes)
    try list.append(allocator, "vendor/picolibc/libc/posix/regcomp.c");
    try list.append(allocator, "vendor/picolibc/libc/posix/regexec.c");
    try list.append(allocator, "vendor/picolibc/libc/posix/regerror.c");
    try list.append(allocator, "vendor/picolibc/libc/posix/regfree.c");
    // C23 <uchar.h> conversions (char8/16/32); internal uchar-local.h is same-dir
    try list.append(allocator, "vendor/picolibc/libc/uchar/mbrtoc8.c");
    try list.append(allocator, "vendor/picolibc/libc/uchar/mbrtoc16.c");
    try list.append(allocator, "vendor/picolibc/libc/uchar/mbrtoc32.c");
    try list.append(allocator, "vendor/picolibc/libc/uchar/c8rtomb.c");
    try list.append(allocator, "vendor/picolibc/libc/uchar/c16rtomb.c");
    try list.append(allocator, "vendor/picolibc/libc/uchar/c32rtomb.c");

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

// libm/common ships ARM optimized-routines transcendentals that collide with
// the classic fdlibm ones in libm/math; let math own them. Only the colliding
// implementation files are dropped (the *_data tables stay, since common-only
// extras such as expm1/log1p/exp10 use them).
const libm_common_exclude = [_][]const u8{
    "cosf.c",   "sinf.c",   "sincosf.c",
    "exp.c",    "exp2.c",   "log.c",    "log2.c",   "pow.c",
    "sf_exp.c", "sf_exp2.c", "sf_log.c", "sf_log2.c", "sf_pow.c", "s_log2.c",
    "exp10l.c", // long double (we ship double+float; ld is the explicit subset)
};

// libm/math ships the deprecated BSD gamma/gammaf, also provided via lgamma.
const libm_math_exclude = [_][]const u8{
    "s_gamma.c", "sf_gamma.c",
};

// Generic soft-float fenv.c is replaced by the x86 machine implementation.
const libm_fenv_exclude = [_][]const u8{
    "fenv.c",
};

// Long-double complex (need long-double elementary fns we don't ship). Listed
// explicitly because a suffix rule would also catch creal.c.
const libm_complex_exclude = [_][]const u8{
    "cabsl.c",   "cacoshl.c", "cacosl.c",  "cargl.c",   "casinhl.c",
    "casinl.c",  "catanhl.c", "catanl.c",  "ccoshl.c",  "ccosl.c",
    "cexpl.c",   "clog10l.c", "clogl.c",   "cpowl.c",   "cprojl.c",
    "csinhl.c",  "csinl.c",   "csqrtl.c",  "ctanhl.c",  "ctanl.c",
    "cephes_subrl.c",
};

// Glob a libm subdir. We ship double + float; long double comes from the
// explicit ld subset, so optionally skip long-double sources: skip_sl drops
// the "sl_*" files (math/common), skip_l_suffix drops the "*l.c" long-double
// complex files (safe in libm/complex, where no double/float name ends in 'l').
fn appendLibmDir(
    b: *std.Build,
    allocator: std.mem.Allocator,
    list: *std.ArrayListUnmanaged([]const u8),
    sub: []const u8,
    exclude_list: []const []const u8,
    skip_sl: bool,
    skip_l_suffix: bool,
) !void {
    const io = b.graph.io;
    var dir = b.build_root.handle.openDir(io, b.fmt("vendor/picolibc/{s}", .{sub}), .{ .iterate = true }) catch return;
    defer dir.close(io);

    var it = dir.iterate();
    dir_loop: while (try it.next(io)) |entry| {
        if (entry.kind != .file) continue;
        if (!std.mem.endsWith(u8, entry.name, ".c")) continue;
        if (skip_sl and std.mem.startsWith(u8, entry.name, "sl_")) continue;
        if (skip_l_suffix and std.mem.endsWith(u8, entry.name, "l.c")) continue;
        for (exclude_list) |skip| {
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
        "libs/libc/xbox/dirio.c",
        "libs/libc/xbox/tmpio.c",
        "libs/libc/xbox/timeio.c",
        "libs/libc/xbox/threads.c",
        "libs/libc/xbox/locks.c",
        "libs/libc/xbox/errno_tls.c",
        "libs/libc/xbox/hooks.c",
        "libs/libc/xbox/signals.c",
        "libs/libc/xbox/startup.c",
        "libs/libc/xbox/stubs.c",
        "libs/libc/xbox/tls_stub.c",
        "libs/libc/xbox/picolibc_aliases.c",
        "libs/libc/xbox/libm_shim.c",
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
    const threads_h = b.addInstallFile(b.path("shared/include/threads.h"), "include/threads.h");
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
    step.dependOn(&threads_h.step);
    step.dependOn(&xbox.step);
    step.dependOn(&xboxkrnl.step);
    step.dependOn(&c23.step);
    return step;
}
