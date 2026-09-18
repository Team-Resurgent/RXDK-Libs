const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");

// vendor/picolibc tracks Team-Resurgent/picolibc's xboxog branch (not upstream), which has had
// every source RXDK never builds deleted outright -- profile-incompatible engine variants (full
// stdio vs our TINY_STDIO profile, Ryu vs the classic dtoa/ftoa/atod/atof engines), anything a
// libs/libc/xbox/*.c replacement fully supersedes (malloc cluster, clock, lock, remove/tmpnam/
// tmpfile, the posixiob_* weak-reference shims), the generic C string routines superseded by
// libc/machine/x86 asm, and long-double math we don't ship. So collectSources below is mostly a
// plain directory glob per subdir.
//
// Two subdirs still need a small exclude list, NOT because we don't want the files -- we do, on
// disk -- but because they're the classic fdlibm/newlib "one algorithm, multiple precisions via
// #include" pattern: a sibling file we DO compile (and DO want as its own translation unit)
// `#include`s them textually to get a differently-named symbol out of the same code, so they must
// never also be compiled as their own TU (duplicate-symbol link error) but must still exist on
// disk for that #include to resolve:
//   - libc/stdio: vfprintf.c includes ultoa_invert.c + vfprintf_{char,float,int,n,str}.c (its
//     per-conversion engine pieces); strtod/strtof/strtold/vfscanf include conv_flt.c;
//     sprintf{d,f}.c/swprintf{d}.c/snprintf{d,f}.c include the bare sprintf/swprintf/snprintf/
//     vsnprintf.c for their own symbol (RXDK's libs/libc/xbox/ms_printf.c separately provides the
//     plain sprintf/swprintf/snprintf/vsnprintf symbols themselves, so this doesn't collide).
//   - libm/common: libm/math/{s_,sf_}{exp,exp2,log,log2,pow}.c each `#include
//     "../common/{exp,exp2,log,log2,pow}.c"` (or the sf_ counterpart) for the actual algorithm;
//     libm/math/sf_{cos,sin,sincos}.c likewise include libm/common/{cosf,sinf,sincosf}.c.
const stdio_include_only = [_][]const u8{
    "conv_flt.c",      "ultoa_invert.c",
    "vfprintf_char.c", "vfprintf_float.c", "vfprintf_int.c", "vfprintf_n.c", "vfprintf_str.c",
    "sprintf.c",       "swprintf.c",       "snprintf.c",     "vsnprintf.c",
};
const libm_common_include_only = [_][]const u8{
    "cosf.c",   "sinf.c",    "sincosf.c",
    "exp.c",    "exp2.c",    "log.c",    "log2.c",   "pow.c",   "s_log2.c",
    "sf_exp.c", "sf_exp2.c", "sf_log.c", "sf_log2.c", "sf_pow.c",
};
// libc/posix files to leave OUT of the glob: engine.c is #included by
// regexec.c (not a standalone TU); the four regex TUs are appended explicitly;
// creat.c / sleep.c collide with fileio.c / threads.c.
const posix_exclude = [_][]const u8{
    "engine.c",
    "regcomp.c", "regexec.c", "regerror.c", "regfree.c",
    "creat.c", "sleep.c",
    // fnmatch lives in xbox/posix_glob.c (bundled with glob, which calls it).
    "fnmatch.c",
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
        const exclude = if (std.mem.eql(u8, sub, "libc/stdio")) &stdio_include_only else &[_][]const u8{};
        try appendDirSources(b, allocator, &list, sub, ".c", exclude);
    }
    // Full double + float math, complex, and fenv; long double stays selective.
    try appendLibmDir(b, allocator, &list, "libm/common", &libm_common_include_only, true, false);
    try appendLibmDir(b, allocator, &list, "libm/math", &[_][]const u8{}, true, false);
    try appendLibmDir(b, allocator, &list, "libm/complex", &[_][]const u8{}, false, false);
    // x86 fenv: use the x87/SSE implementation (libm/machine/x86/fenv.c); the
    // generic soft-float fenv.c doesn't exist on our fork. The other helpers
    // (fe_dfl_env, fegetmode, fesetmode) are arch-generic and stay.
    try appendLibmDir(b, allocator, &list, "libm/fenv", &[_][]const u8{}, false, false);
    try list.append(allocator, "vendor/picolibc/libm/machine/x86/fenv.c");
    for (libm_ld_sources) |src| {
        try list.append(allocator, src);
    }
    try list.append(allocator, "libs/libc/xbox/posix_stdio_streams.c");
    // x86 setjmp/longjmp (machine asm; needs <picolibc.h> + relative i386mach.h)
    try list.append(allocator, "vendor/picolibc/libc/machine/x86/setjmp.S");
    // x86 string/memory routines, in place of the generic C ones excluded above
    for ([_][]const u8{ "memchr", "memcmp", "memcpy", "memmove", "memset", "strchr", "strlen" }) |name| {
        const rel = try std.fmt.allocPrint(allocator, "vendor/picolibc/libc/machine/x86/{s}.S", .{name});
        try list.append(allocator, rel);
    }
    // POSIX regex (Henry Spencer engine; internal headers are same-dir quote includes)
    try list.append(allocator, "vendor/picolibc/libc/posix/regcomp.c");
    try list.append(allocator, "vendor/picolibc/libc/posix/regexec.c");
    try list.append(allocator, "vendor/picolibc/libc/posix/regerror.c");
    try list.append(allocator, "vendor/picolibc/libc/posix/regfree.c");
    // The rest of libc/posix: strfmon, fnmatch, basename/dirname, the exec*
    // front-ends, wait(), and the passwd/group DB (getpwuid/getgrgid/...). Built
    // as picolibc's own meson builds it -- a consistent set. Excludes: engine.c
    // (#included by regexec.c, not a standalone TU); the 4 regex TUs appended
    // above; and creat.c / sleep.c (defined in fileio.c / threads.c here).
    try appendDirSources(b, allocator, &list, "libc/posix", ".c", &posix_exclude);
    // C23 <uchar.h> conversions (char8/16/32); internal uchar-local.h is same-dir
    try list.append(allocator, "vendor/picolibc/libc/uchar/mbrtoc8.c");
    try list.append(allocator, "vendor/picolibc/libc/uchar/mbrtoc16.c");
    try list.append(allocator, "vendor/picolibc/libc/uchar/mbrtoc32.c");
    try list.append(allocator, "vendor/picolibc/libc/uchar/c8rtomb.c");
    try list.append(allocator, "vendor/picolibc/libc/uchar/c16rtomb.c");
    try list.append(allocator, "vendor/picolibc/libc/uchar/c32rtomb.c");
    // POSIX sysconf (fallback impl, weak-aliased to sysconf). libc++'s
    // thread::hardware_concurrency() calls sysconf(_SC_NPROCESSORS_ONLN); our
    // fork returns 1 (the OG Xbox is single-core).
    try list.append(allocator, "vendor/picolibc/libos/fallback/sysconf.c");

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
        const rel = try std.fmt.allocPrint(allocator, "vendor/picolibc/{s}/{s}", .{ sub, entry.name });
        try list.append(allocator, rel);
    }
}

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
        "vendor/picolibc/libc/include",
        "vendor/picolibc/libc/machine/x86",
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
        "libs/libc/xbox/heapalloc.c",
        "libs/libc/xbox/fileio.c",
        "libs/libc/xbox/dirio.c",
        "libs/libc/xbox/tmpio.c",
        "libs/libc/xbox/timeio.c",
        "libs/libc/xbox/threads.c",
        "libs/libc/xbox/emutls.c",
        "libs/libc/xbox/locks.c",
        "libs/libc/xbox/errno_tls.c",
        "libs/libc/xbox/hooks.c",
        "libs/libc/xbox/signals.c",
        "libs/libc/xbox/posix_ext.c",
        "libs/libc/xbox/posix_glob.c",
        "libs/libc/xbox/aio.c",
        "libs/libc/xbox/syslog.c",
        "libs/libc/xbox/pthread.c",
        "libs/libc/xbox/sem.c",
        "libs/libc/xbox/ftw.c",
        "libs/libc/xbox/startup.c",
        "libs/libc/xbox/xbld.c",
        "libs/libc/xbox/stubs.c",
        "libs/libc/xbox/tls_stub.c",
        "libs/libc/xbox/libm_shim.c",
        "libs/libc/xbox/msvc_crt.c",
        "libs/libc/xbox/ms_crt_compat.c",
        "libs/libc/xbox/cmdline.c",
        "libs/libc/xbox/ms_printf.c",
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

// Non-header files to keep out of the staged/dist include tree (build-system
// cruft + stray sources + docs). exclude_extensions matches the file extension,
// so extensionless C++ headers (vector, __config, ...) are unaffected.
const header_excludes = [_][]const u8{ ".txt", ".build", ".in", ".c", ".md", ".cmake", ".am" };

pub fn stageHeaders(b: *std.Build) *std.Build.Step {
    const install = b.addInstallDirectory(.{
        .source_dir = b.path("vendor/picolibc/libc/include"),
        .install_dir = .prefix,
        .install_subdir = "include",
        .exclude_extensions = &header_excludes,
    });
    const gen = b.addInstallFile(b.path("build/generated/picolibc.h"), "include/picolibc.h");
    const threads_h = b.addInstallFile(b.path("shared/include/threads.h"), "include/threads.h");
    const xbox = b.addInstallDirectory(.{
        .source_dir = b.path("shared/include/xbox"),
        .install_dir = .prefix,
        .install_subdir = "include/xbox",
        .exclude_extensions = &header_excludes,
    });
    const xboxkrnl = b.addInstallDirectory(.{
        .source_dir = b.path("shared/include/xboxkrnl"),
        .install_dir = .prefix,
        .install_subdir = "include/xboxkrnl",
        .exclude_extensions = &header_excludes,
    });
    const c23 = b.addInstallDirectory(.{
        .source_dir = b.path("libs/libc/c23"),
        .install_dir = .prefix,
        .install_subdir = "include",
        .exclude_extensions = &header_excludes,
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
