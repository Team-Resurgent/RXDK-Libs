const std = @import("std");

const xbox_target = @import("build/xbox_target.zig");
const coff_lib = @import("build/coff_lib.zig");
const picolibc = @import("libs/libc/build.zig");
const libcxx = @import("libs/libcpp/build.zig");
const libunwind = @import("libs/libcpp/unwind.zig");
const libxapi_pkg = @import("libs/libxapi/build.zig");
const compile_c = @import("build/compile_c.zig");
const link_pe = @import("build/link_pe.zig");
const verify_no_vs = @import("build/verify_no_vs.zig");

fn optFlag(optimize: std.builtin.OptimizeMode) []const u8 {
    return switch (optimize) {
        .Debug => "-O0",
        .ReleaseSafe => "-O2",
        .ReleaseFast => "-O3",
        .ReleaseSmall => "-Os",
    };
}

pub fn build(b: *std.Build) void {
    const target = xbox_target.resolveTarget(b);
    const optimize = b.standardOptimizeOption(.{});
    const opt_flag = optFlag(optimize);

    const verify = verify_no_vs.addStep(b);
    const verify_step = b.step("verify-no-vs", "Assert build/*.zig never invokes MSVC toolchain");
    verify_step.dependOn(verify);

    const mkdir_lib = b.addWriteFiles();
    _ = mkdir_lib.add("zig-out/lib/.keep", "");
    const mkdir_samples = b.addWriteFiles();
    _ = mkdir_samples.add("zig-out/samples/.keep", "");

    const picolibc_objs = picolibc.addPicolibcObjects(b, xbox_target, opt_flag) catch @panic("picolibc sources");
    const xbox_objs = picolibc.addXboxObjects(b, xbox_target, opt_flag);

    var libc_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    libc_objects.appendSlice(b.allocator, picolibc_objs.outputs) catch @panic("OOM");
    libc_objects.appendSlice(b.allocator, xbox_objs.outputs) catch @panic("OOM");

    var libc_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    libc_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    libc_deps.append(b.allocator, picolibc_objs.step) catch @panic("OOM");
    libc_deps.append(b.allocator, xbox_objs.step) catch @panic("OOM");

    const libc = coff_lib.pack(b, "libc", libc_objects.items, libc_deps.items);
    const install_libc = b.addInstallFile(libc.path, "lib/libc.lib");
    install_libc.step.dependOn(libc.step);

    const stage_c_headers = picolibc.stageHeaders(b);
    stage_c_headers.dependOn(&install_libc.step);

    const libcxx_objs = libcxx.addLibcxxObjects(b, xbox_target, opt_flag) catch @panic("libcxx sources");

    // LLVM libunwind (DWARF) is an internal component of the C++ runtime, so
    // its objects are packed into libcpp.lib (not a separate public lib). Until
    // exceptions are enabled nothing references _Unwind_*, so archive semantics
    // keep these objects out of links (and thus no undefined __eh_frame_start).
    const libunwind_objs = libunwind.addUnwindObjects(b, opt_flag);
    const libunwind_step = b.step("libunwind", "Build LLVM libunwind (DWARF unwinder) objects");
    libunwind_step.dependOn(libunwind_objs.step);

    var libcpp_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    libcpp_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    libcpp_deps.append(b.allocator, libcxx_objs.step) catch @panic("OOM");
    libcpp_deps.append(b.allocator, libunwind_objs.step) catch @panic("OOM");
    libcpp_deps.append(b.allocator, libc.step) catch @panic("OOM");
    libcpp_deps.append(b.allocator, xbox_objs.step) catch @panic("OOM");

    var libcpp_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    libcpp_objects.appendSlice(b.allocator, libcxx_objs.outputs) catch @panic("OOM");
    libcpp_objects.appendSlice(b.allocator, libunwind_objs.outputs) catch @panic("OOM");

    const libcpp = coff_lib.pack(b, "libcpp", libcpp_objects.items, libcpp_deps.items);
    const install_libcpp = b.addInstallFile(libcpp.path, "lib/libcpp.lib");
    install_libcpp.step.dependOn(libcpp.step);

    const stage_cxx_headers = libcxx.stageHeaders(b);
    stage_cxx_headers.dependOn(&install_libcpp.step);

    const xapi_objs = libxapi_pkg.addAllObjects(b, xbox_target, opt_flag);
    var xapi_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xapi_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xapi_deps.append(b.allocator, xapi_objs.step) catch @panic("OOM");
    const libxapi = coff_lib.pack(b, "libxapi", xapi_objs.outputs, xapi_deps.items);
    const install_libxapi = b.addInstallFile(libxapi.path, "lib/libxapi.lib");
    install_libxapi.step.dependOn(libxapi.step);

    var xapi_core_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    for (xapi_objs.outputs) |obj| {
        const path = obj.getPath(b);
        if (libxapi_pkg.isCoreObjectPath(path)) {
            xapi_core_objects.append(b.allocator, obj) catch @panic("OOM");
        }
    }
    var xapi_core_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xapi_core_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xapi_core_deps.append(b.allocator, xapi_objs.step) catch @panic("OOM");
    const libxapi_core = coff_lib.pack(b, "libxapi_core", xapi_core_objects.items, xapi_core_deps.items);
    const install_libxapi_core = b.addInstallFile(libxapi_core.path, "lib/libxapi_core.lib");
    install_libxapi_core.step.dependOn(libxapi_core.step);

    const stage_xapi_headers = libxapi_pkg.stageHeaders(b);
    stage_xapi_headers.dependOn(&install_libxapi.step);

    b.getInstallStep().dependOn(&install_libc.step);
    b.getInstallStep().dependOn(&install_libcpp.step);
    b.getInstallStep().dependOn(&install_libxapi.step);
    b.getInstallStep().dependOn(stage_c_headers);
    b.getInstallStep().dependOn(stage_cxx_headers);
    b.getInstallStep().dependOn(stage_xapi_headers);
    b.getInstallStep().dependOn(verify);

    const libc_step = b.step("libc", "Build libc.lib (picolibc + Xbox HAL)");
    libc_step.dependOn(libc.step);
    const libcpp_step = b.step("libcpp", "Build libcpp.lib (libc++)");
    libcpp_step.dependOn(libcpp.step);
    const libxapi_step = b.step("libxapi", "Build libxapi.lib (full xAPI)");
    libxapi_step.dependOn(libxapi.step);
    const libxapi_core_step = b.step("libxapi-core", "Build libxapi_core.lib (k32+dll+rtl+uuid, no USB)");
    libxapi_core_step.dependOn(libxapi_core.step);
    const xapi_slices_step = b.step("xapi-slices", "Compile all libxapi source slices to objects");
    xapi_slices_step.dependOn(xapi_objs.step);

    const krnl = b.path("prebuilt/xboxkrnl.lib");

    var sample_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    sample_objects.appendSlice(b.allocator, picolibc_objs.outputs) catch @panic("OOM");
    sample_objects.appendSlice(b.allocator, xbox_objs.outputs) catch @panic("OOM");

    const libxapi_lib = b.path("zig-out/lib/libxapi.lib");
    const xapi_inc = [_]std.Build.LazyPath{
        b.path("shared/include"),
        b.path("libs/libxapi/xapi/internal"),
        b.path("build/generated"),
        b.path("shared/picolibc/include"),
        b.path("shared/picolibc/machine/x86"),
    };

    const xapi_smoke_extra = [_][]const u8{
        "samples/xapi-smoke/src/xapi_boot.c",
        "samples/xapi-smoke/src/common.c",
        "samples/xapi-smoke/src/test_content.c",
        "samples/xapi-smoke/src/test_copyfile.c",
        "samples/xapi-smoke/src/test_devices.c",
        "samples/xapi-smoke/src/test_dirs.c",
        "samples/xapi-smoke/src/test_error.c",
        "samples/xapi-smoke/src/test_fiber.c",
        "samples/xapi-smoke/src/test_files.c",
        "samples/xapi-smoke/src/test_find.c",
        "samples/xapi-smoke/src/test_handle.c",
        "samples/xapi-smoke/src/test_interlocked.c",
        "samples/xapi-smoke/src/test_memory.c",
        "samples/xapi-smoke/src/test_mu.c",
        "samples/xapi-smoke/src/test_nickname.c",
        "samples/xapi-smoke/src/test_path.c",
        "samples/xapi-smoke/src/test_savegame.c",
        "samples/xapi-smoke/src/test_section.c",
        "samples/xapi-smoke/src/test_strings.c",
        "samples/xapi-smoke/src/test_sync.c",
        "samples/xapi-smoke/src/test_sync2.c",
        "samples/xapi-smoke/src/test_threads.c",
        "samples/xapi-smoke/src/test_time.c",
        "samples/xapi-smoke/src/test_tls.c",
        "samples/xapi-smoke/src/test_virtual.c",
        "samples/xapi-smoke/src/test_widechar.c",
        "samples/xapi-smoke/src/test_xbox.c",
        "samples/xapi-smoke/src/test_xinput.c",
    };

    const xapi_smoke = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "xapi-smoke",
        .src = "samples/xapi-smoke/src/main.c",
        .extra_srcs = &xapi_smoke_extra,
        .objects = sample_objects.items,
        .libs = &.{ libxapi_lib, krnl },
        .include_paths = &xapi_inc,
        .extra_flags = &.{
            "-D_XAPI_",
            "-fms-extensions",
            "-fms-compatibility",
            "-include",
            "build/generated/picolibc.h",
        },
        .entry = "xapi_smoke_boot_entry",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, libxapi.step, picolibc_objs.step, xbox_objs.step },
    });
    const xapi_smoke_step = b.step("xapi-smoke", "Build xAPI category smoke tests (27 tests, kit hardware)");
    xapi_smoke_step.dependOn(xapi_smoke.install);

    var cpp_sample_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    cpp_sample_objects.appendSlice(b.allocator, picolibc_objs.outputs) catch @panic("OOM");
    cpp_sample_objects.appendSlice(b.allocator, xbox_objs.outputs) catch @panic("OOM");
    cpp_sample_objects.appendSlice(b.allocator, libcxx_objs.outputs) catch @panic("OOM");
    cpp_sample_objects.appendSlice(b.allocator, libunwind_objs.outputs) catch @panic("OOM");

    const cxx_inc = [_]std.Build.LazyPath{
        b.path("build/generated/libcxx"),
        b.path("shared/libcxx/include"),
        b.path("build/generated"),
        b.path("shared/include"),
        b.path("shared/picolibc/include"),
        b.path("shared/picolibc/machine/x86"),
    };

    const libc_smoke = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "libc-smoke",
        .src = "samples/libc-smoke/main.c",
        .extra_srcs = &.{"samples/libc-smoke/generated_tests.c"},
        .objects = sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &.{ b.path("shared/include"), b.path("build/generated"), b.path("shared/picolibc/include") },
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, picolibc_objs.step, xbox_objs.step },
    });
    const libc_smoke_step = b.step("libc-smoke", "Build libc / C23 runtime smoke (kit ISO)");
    libc_smoke_step.dependOn(libc_smoke.install);

    // libcpp-smoke links the libc objects too (libc++ -> libc): cpp_sample_objects
    // already bundles picolibc + xbox (libc) alongside the libcxx objects.
    const libcpp_smoke = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "libcpp-smoke",
        .src = "samples/libcpp-smoke/main.cpp",
        .extra_srcs = &.{"samples/libcpp-smoke/tests.cpp"},
        .is_cpp = true,
        .objects = cpp_sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &cxx_inc,
        .extra_flags = &.{
            "-U_WIN32",
            "-U__MINGW32__",
            "-fexceptions",
            "-include", "picolibc_prereq.h",
            "-include", "__config_site",
        },
        .entry = "start",
        .bootstrap = true,
        .eh_frame_bracket = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, libcpp.step, picolibc_objs.step, xbox_objs.step, libcxx_objs.step, libunwind_objs.step },
    });
    const libcpp_smoke_step = b.step("libcpp-smoke", "Build libc++ / C++23 runtime smoke (kit ISO)");
    libcpp_smoke_step.dependOn(libcpp_smoke.install);
}
