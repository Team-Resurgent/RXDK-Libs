const std = @import("std");
const compile_c = @import("compile_c.zig");

const exclude = [_][]const u8{
    "filesystem",
    "random",
    "regex",
    "thread",
    "mutex",
    "shared_mutex",
    "condition_variable",
    "future",
    "chrono",
    "barrier",
    "latch",
    "semaphore",
    "stop_token",
    "strstream",
    "atomic.cpp",
    "charconv",
    "algorithm.cpp",
    "experimental/",
    "support/",
    "pstl/",
    "ryu/",
    "print.cpp",
    "memory_resource",
    "hash.cpp",
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
    "cxa_noexception.cpp",
    "cxa_exception_storage.cpp",
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

    for (cxxabi_sources) |src| {
        const rel = try std.fmt.allocPrint(allocator, "vendor/llvm-project/libcxxabi/src/{s}", .{src});
        try list.append(allocator, rel);
    }

    return try list.toOwnedSlice(allocator);
}

fn includeDirs(_: *std.Build) []const []const u8 {
    return &.{
        "build/generated/libcxx",
        "vendor/llvm-project/libcxx/include",
        "vendor/llvm-project/libcxx/src/include",
        "vendor/llvm-project/libcxxabi/include",
        "vendor/llvm-project/libcxx/src",
        "build/generated",
        "include",
        "vendor/picolibc/libc/include",
        "vendor/picolibc/libc/machine/x86",
    };
}

pub fn addLibcxxObjects(
    b: *std.Build,
    xbox_target: @TypeOf(@import("xbox_target.zig")),
    opt_flag: []const u8,
) !compile_c.CompileBatch {
    const sources = try collectLibcxxSources(b, b.allocator);
    const flags = xbox_target.appendFlags(b, xbox_target.cppFlags(b), &.{
        "-U_WIN32",
        "-U__MINGW32__",
        "-include",
        "picolibc_prereq.h",
        "-include",
        "__config_site",
        "-D_LIBCPP_BUILDING_LIBRARY",
        "-DLIBCXX_BUILDING_LIBCXXABI",
        "-DLIBCXXABI_BUILDING_LIBCXXABI",
        "-D_LIBCXXABI_HAS_NO_THREADS",
        "-D_LIBCPP_LIBC_NEWLIB",
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
        .source_dir = b.path("vendor/llvm-project/libcxx/include"),
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
