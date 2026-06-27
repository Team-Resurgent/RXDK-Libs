const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");

const exclude = [_][]const u8{
    "filesystem",
    "random_shuffle",
    "barrier",
    "latch",
    "semaphore",
    "stop_token",
    "strstream",
    "atomic.cpp",
    // (thread.cpp / mutex.cpp / shared_mutex.cpp / condition_variable*.cpp are
    //  enabled: C11 thread API backed by libc <threads.h>.)
    // charconv.cpp instantiates both to_chars AND from_chars for floating point;
    // from_chars_floating_point.h needs src/include/shared/*.h, which this libc++
    // snapshot does not vendor. We only need FP to_chars (std::format/print output),
    // so charconv.cpp stays excluded and libs/libcpp/charconv_fp_to_chars.cpp
    // (added below) instantiates just the to_chars overloads, backed by ryu/*.cpp.
    "charconv",
    "experimental/",
    "support/",
    "pstl/",
    "memory_resource",
    "valarray",
    "bind.cpp",
    "fstream",
    "new.cpp",
    "cxa_thread_atexit.cpp",
};

const cxxabi_sources = [_][]const u8{
    "abort_message.cpp",
    "stdlib_stdexcept.cpp",
    "stdlib_typeinfo.cpp",
    "stdlib_exception.cpp",
    "stdlib_new_delete.cpp",
    "cxa_aux_runtime.cpp",
    "cxa_default_handlers.cpp",
    "cxa_exception.cpp",
    "cxa_personality.cpp",
    "cxa_exception_storage.cpp",
    "fallback_malloc.cpp",
    "cxa_handlers.cpp",
    "cxa_vector.cpp",
    "cxa_virtual.cpp",
    "cxa_guard.cpp",
    "cxa_demangle.cpp",
    "private_typeinfo.cpp",
};

pub fn collectLibcxxSources(b: *std.Build, allocator: std.mem.Allocator) ![]const []const u8 {
    var list = std.ArrayListUnmanaged([]const u8).empty;
    errdefer list.deinit(allocator);

    const io = b.graph.io;
    var dir = try b.build_root.handle.openDir(io, "vendor/llvm-project/libcxx/src", .{ .iterate = true });
    defer dir.close(io);

    var it = dir.iterate();
    while (try it.next(io)) |entry| {
        if (entry.kind != .file) continue;
        if (!std.mem.endsWith(u8, entry.name, ".cpp")) continue;
        var skip = false;
        for (exclude) |pat| {
            if (std.mem.indexOf(u8, entry.name, pat) != null) {
                skip = true;
                break;
            }
        }
        if (skip) continue;
        const rel = try std.fmt.allocPrint(allocator, "vendor/llvm-project/libcxx/src/{s}", .{entry.name});
        try list.append(allocator, rel);
    }

    // <filesystem> sources live in a subdir (not covered by the glob above).
    // Enabled via _LIBCPP_HAS_FILESYSTEM; backed by libc dirio.c (dirent + path
    // ops over Nt*). Symlink/hardlink ops degrade to ENOSYS (FATX has neither).
    var fs_dir = try b.build_root.handle.openDir(io, "vendor/llvm-project/libcxx/src/filesystem", .{ .iterate = true });
    defer fs_dir.close(io);
    var fs_it = fs_dir.iterate();
    while (try fs_it.next(io)) |entry| {
        if (entry.kind != .file) continue;
        if (!std.mem.endsWith(u8, entry.name, ".cpp")) continue;
        const rel = try std.fmt.allocPrint(allocator, "vendor/llvm-project/libcxx/src/filesystem/{s}", .{entry.name});
        try list.append(allocator, rel);
    }

    // ryu/*.cpp (Ryu float<->string) backs charconv.cpp's floating-point
    // to_chars/from_chars. Also a subdir, so walked explicitly like filesystem.
    var ryu_dir = try b.build_root.handle.openDir(io, "vendor/llvm-project/libcxx/src/ryu", .{ .iterate = true });
    defer ryu_dir.close(io);
    var ryu_it = ryu_dir.iterate();
    while (try ryu_it.next(io)) |entry| {
        if (entry.kind != .file) continue;
        if (!std.mem.endsWith(u8, entry.name, ".cpp")) continue;
        const rel = try std.fmt.allocPrint(allocator, "vendor/llvm-project/libcxx/src/ryu/{s}", .{entry.name});
        try list.append(allocator, rel);
    }

    for (cxxabi_sources) |src| {
        const rel = try std.fmt.allocPrint(allocator, "vendor/llvm-project/libcxxabi/src/{s}", .{src});
        try list.append(allocator, rel);
    }

    // First-party FP to_chars (see charconv exclusion note above).
    try list.append(allocator, try allocator.dupe(u8, "libs/libcpp/charconv_fp_to_chars.cpp"));

    return try list.toOwnedSlice(allocator);
}

fn includeDirs(_: *std.Build) []const []const u8 {
    return &.{
        "build/generated/libcxx",
        "shared/libcxx/include",
        "vendor/llvm-project/libcxx/src/include",
        "vendor/llvm-project/libcxxabi/include",
        "vendor/llvm-project/libcxx/src",
        "build/generated",
        "shared/include", // libc <threads.h> for the C11 thread API backend
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
    };
}

pub fn addLibcxxObjects(
    b: *std.Build,
    xbox_target: @TypeOf(@import("../../build/xbox_target.zig")),
    opt_flag: []const u8,
) !compile_c.CompileBatch {
    const sources = try collectLibcxxSources(b, b.allocator);
    const flags = xbox_target.appendFlags(b, xbox_target.cppFlags(b), &.{
        // C++ exceptions: DWARF EH backed by libunwind. Overrides cppFlags'
        // -fno-exceptions (last flag wins) so the runtime's throw machinery
        // (__cxa_throw, the personality routine) is generated.
        "-fexceptions",
        // Vendored libc++/libcxxabi — silence upstream warnings (e.g.
        // -Wpragma-clang-attribute), matching the picolibc/libxapi batches.
        "-Wno-everything",
        "-U_WIN32",
        "-U__MINGW32__",
        // We undefine _WIN32 above to take libc++/libcxxabi's Itanium/POSIX code
        // paths, but the exception-object destructor must still be called with
        // the target's member-function convention (__thiscall on i386: this in
        // ECX). Without this, libcxxabi calls ~T() as cdecl and the destructor
        // reads `this` from a stale ECX -> wild pointer (e.g. ~runtime_error's
        // refstring). Keep the __thiscall qualifier on _LIBCXXABI_DTOR_FUNC.
        "-D_LIBCXXABI_FORCE_THISCALL_DTOR",
        "-include",
        "picolibc_prereq.h",
        "-include",
        "__config_site",
        "-D_LIBCPP_BUILDING_LIBRARY",
        "-DLIBCXX_BUILDING_LIBCXXABI",
        "-DLIBCXXABI_BUILDING_LIBCXXABI",
        "-D_LIBCXXABI_HAS_NO_THREADS",
        "-D_LIBCPP_LIBC_NEWLIB",
        // Our libc provides clock_gettime with CLOCK_REALTIME + CLOCK_MONOTONIC
        // (timeio.c); tell <chrono> to use it for system_clock + steady_clock.
        // Not auto-detected because picolibc doesn't advertise _POSIX_TIMERS.
        "-D_LIBCPP_HAS_CLOCK_GETTIME",
        // random_device routes to our getentropy() (stubs.c) via the
        // /dev/urandom token; pick that backend explicitly.
        "-D_LIBCPP_USING_GETENTROPY",
    });
    return compile_c.addBatch(b, .{
        .name = "libcxx",
        .target = xbox_target.target_triple,
        .out_subdir = "libcxx",
        .sources = sources,
        .flags = flags,
        .include_dirs = includeDirs(b),
        .opt_flag = opt_flag,
    });
}

pub fn stageHeaders(b: *std.Build) *std.Build.Step {
    const cxx = b.addInstallDirectory(.{
        .source_dir = b.path("shared/libcxx/include"),
        .install_dir = .prefix,
        .install_subdir = "include/c++/v1",
    });
    const site = b.addInstallFile(b.path("build/generated/libcxx/__config_site"), "include/c++/v1/__config_site");
    const assert_handler = b.addInstallFile(
        b.path("build/generated/libcxx/__assertion_handler"),
        "include/c++/v1/__assertion_handler",
    );

    const step = b.allocator.create(std.Build.Step) catch @panic("OOM");
    step.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "stage-libcxx-headers",
        .owner = b,
    });
    step.dependOn(&cxx.step);
    step.dependOn(&site.step);
    step.dependOn(&assert_handler.step);
    return step;
}
