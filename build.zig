const std = @import("std");

const xbox_target = @import("build/xbox_target.zig");
const coff_lib = @import("build/coff_lib.zig");
const picolibc = @import("build/picolibc.zig");
const libcxx = @import("build/libcxx.zig");
const xapi = @import("build/xapi.zig");
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

    var libcpp_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    libcpp_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    libcpp_deps.append(b.allocator, libcxx_objs.step) catch @panic("OOM");
    libcpp_deps.append(b.allocator, libc.step) catch @panic("OOM");
    libcpp_deps.append(b.allocator, xbox_objs.step) catch @panic("OOM");

    const libcpp = coff_lib.pack(b, "libcpp", libcxx_objs.outputs, libcpp_deps.items);
    const install_libcpp = b.addInstallFile(libcpp.path, "lib/libcpp.lib");
    install_libcpp.step.dependOn(libcpp.step);

    const stage_cxx_headers = libcxx.stageHeaders(b);
    stage_cxx_headers.dependOn(&install_libcpp.step);

    const xapi_objs = xapi.addAllObjects(b, xbox_target, opt_flag);
    var xapi_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xapi_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xapi_deps.append(b.allocator, xapi_objs.step) catch @panic("OOM");
    const libxapi = coff_lib.pack(b, "libxapi", xapi_objs.outputs, xapi_deps.items);
    const install_libxapi = b.addInstallFile(libxapi.path, "lib/libxapi.lib");
    install_libxapi.step.dependOn(libxapi.step);

    var xapi_core_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    for (xapi_objs.outputs) |obj| {
        const path = obj.getPath(b);
        const in_core =
            std.mem.indexOf(u8, path, "/xapi/k32/") != null or
            std.mem.indexOf(u8, path, "\\xapi\\k32\\") != null or
            std.mem.indexOf(u8, path, "/xapi/dll/") != null or
            std.mem.indexOf(u8, path, "\\xapi\\dll\\") != null or
            std.mem.indexOf(u8, path, "/xapi/rtl/") != null or
            std.mem.indexOf(u8, path, "\\xapi\\rtl\\") != null or
            std.mem.indexOf(u8, path, "/xapi/uuid/") != null or
            std.mem.indexOf(u8, path, "\\xapi\\uuid\\") != null;
        if (in_core) {
            xapi_core_objects.append(b.allocator, obj) catch @panic("OOM");
        }
    }
    var xapi_core_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xapi_core_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xapi_core_deps.append(b.allocator, xapi_objs.step) catch @panic("OOM");
    const libxapi_core = coff_lib.pack(b, "libxapi_core", xapi_core_objects.items, xapi_core_deps.items);
    const install_libxapi_core = b.addInstallFile(libxapi_core.path, "lib/libxapi_core.lib");
    install_libxapi_core.step.dependOn(libxapi_core.step);

    const stage_xapi_headers = xapi.stageHeaders(b);
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
    const inc = [_]std.Build.LazyPath{
        b.path("include"),
        b.path("build/generated"),
        b.path("vendor/picolibc/libc/include"),
        b.path("vendor/picolibc/libc/machine/x86"),
    };

    const kernel = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "kernel-smoke",
        .src = "samples/kernel-smoke/main.c",
        .libs = &.{krnl},
        .include_paths = &inc,
        .entry = "_start",
        .deps = &.{ verify, &mkdir_samples.step },
    });
    const kernel_step = b.step("kernel-smoke", "Build kernel-only smoke PE");
    kernel_step.dependOn(kernel.install);

    var sample_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    sample_objects.appendSlice(b.allocator, picolibc_objs.outputs) catch @panic("OOM");
    sample_objects.appendSlice(b.allocator, xbox_objs.outputs) catch @panic("OOM");

    const kernel_api = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "kernel-api-smoke",
        .src = "samples/kernel-api-smoke/main.c",
        .objects = sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &inc,
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, picolibc_objs.step, xbox_objs.step },
    });
    const kernel_api_step = b.step("kernel-api-smoke", "Build KeQuerySystemTime + stdio smoke PE");
    kernel_api_step.dependOn(kernel_api.install);

    const kernel_api_link = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "kernel-api-link",
        .src = "samples/kernel-api-link/main.c",
        .extra_srcs = &.{"samples/kernel-api-link/generated_link.c"},
        .objects = sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &inc,
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, picolibc_objs.step, xbox_objs.step },
    });
    const kernel_api_link_step = b.step("kernel-api-link", "Build link coverage for all xboxkrnl exports");
    kernel_api_link_step.dependOn(kernel_api_link.install);

    // Kit runtime probe: all non-debug exports in one XBE (see generated_probes_kit.c).
    const probe_sample = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "kernel-api-probe",
        .src = "samples/kernel-api-probe/main.c",
        .extra_srcs = &.{"samples/kernel-api-probe/generated_probes_kit.c"},
        .objects = sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &inc,
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, picolibc_objs.step, xbox_objs.step },
    });
    const kernel_api_probe_step = b.step("kernel-api-probe", "Build full kernel API runtime probe (360 exports, kit ISO)");
    kernel_api_probe_step.dependOn(probe_sample.install);

    const probe_debug = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "kernel-api-probe-debug",
        .src = "samples/kernel-api-probe/main.c",
        .extra_srcs = &.{"samples/kernel-api-probe/generated_probes_debug.c"},
        .objects = sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &inc,
        .extra_flags = &.{"-DKERNEL_API_PROBE_DEBUG=1"},
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, picolibc_objs.step, xbox_objs.step },
    });
    const kernel_api_probe_debug_step = b.step("kernel-api-probe-debug", "Build DbgLoad/UnLoad + MmDbg* probes (debug kernel / BFM debug BIOS only)");
    kernel_api_probe_debug_step.dependOn(probe_debug.install);
    kernel_api_probe_step.dependOn(kernel_api_probe_debug_step);

    const libxapi_lib = b.path("zig-out/lib/libxapi.lib");
    const libxapi_core_lib = b.path("zig-out/lib/libxapi_core.lib");

    const xapi_link = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "xapi-link",
        .src = "samples/xapi-link/main.c",
        .objects = sample_objects.items,
        .libs = &.{ libxapi_lib, krnl },
        .include_paths = &inc,
        .extra_flags = &.{},
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, libxapi.step, picolibc_objs.step, xbox_objs.step },
    });
    const xapi_link_step = b.step("xapi-link", "Build libc + libxapi.lib link smoke PE");
    xapi_link_step.dependOn(xapi_link.install);

    const xapi_inc = [_]std.Build.LazyPath{
        b.path("include"),
        b.path("include/xapi"),
        b.path("include/xdk"),
        b.path("include/sdk"),
        b.path("include/xboxkrnl"),
        b.path("build/generated"),
        b.path("vendor/picolibc/libc/include"),
        b.path("vendor/picolibc/libc/machine/x86"),
    };

    const xapi_smoke_extra = [_][]const u8{
        "samples/xapi-smoke/src/xapi_boot.c",
        "samples/xapi-smoke/src/link_stubs.c",
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
        .libs = &.{ libxapi_core_lib, krnl },
        .include_paths = &xapi_inc,
        .extra_flags = &.{
            "-D_XAPI_",
            "-fms-extensions",
            "-fms-compatibility",
            "-include",
            "picolibc.h",
            "-include",
            "xapi_title_site.h",
        },
        .entry = "xapi_smoke_boot_entry",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, libxapi_core.step, picolibc_objs.step, xbox_objs.step },
    });
    const xapi_smoke_step = b.step("xapi-smoke", "Build xAPI category smoke tests (27 tests, kit hardware)");
    xapi_smoke_step.dependOn(xapi_smoke.install);

    var cpp_sample_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    cpp_sample_objects.appendSlice(b.allocator, picolibc_objs.outputs) catch @panic("OOM");
    cpp_sample_objects.appendSlice(b.allocator, xbox_objs.outputs) catch @panic("OOM");
    cpp_sample_objects.appendSlice(b.allocator, libcxx_objs.outputs) catch @panic("OOM");

    const hello_c = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "hello-c",
        .src = "samples/hello-c/main.c",
        .objects = sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &inc,
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, picolibc_objs.step, xbox_objs.step },
    });
    const hello_c_step = b.step("hello-c", "Build hello-c sample PE");
    hello_c_step.dependOn(hello_c.install);

    const cxx_inc = [_]std.Build.LazyPath{
        b.path("build/generated/libcxx"),
        b.path("vendor/llvm-project/libcxx/include"),
        b.path("build/generated"),
        b.path("include"),
        b.path("vendor/picolibc/libc/include"),
        b.path("vendor/picolibc/libc/machine/x86"),
    };

    const hello_cpp = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "hello-cpp",
        .src = "samples/hello-cpp/main.cpp",
        .is_cpp = true,
        .objects = cpp_sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &cxx_inc,
        .extra_flags = &.{
            "-U_WIN32",
            "-U__MINGW32__",
            "-include", "picolibc_prereq.h",
            "-include", "__config_site",
        },
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, libcpp.step, picolibc_objs.step, xbox_objs.step, libcxx_objs.step },
    });
    const hello_cpp_step = b.step("hello-cpp", "Build hello-cpp sample PE");
    hello_cpp_step.dependOn(hello_cpp.install);

    const c23 = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "c23-stdbit-smoke",
        .src = "samples/conformance/c23/stdbit_smoke.c",
        .objects = sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &.{ b.path("include"), b.path("build/generated"), b.path("src/runtime/c23"), b.path("vendor/picolibc/libc/include") },
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, picolibc_objs.step, xbox_objs.step },
    });
    const c23_step = b.step("conformance-c23", "Build C23 conformance smokes");
    c23_step.dependOn(c23.install);

    const cpp23 = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "cpp23-expected-smoke",
        .src = "samples/conformance/cpp23/expected_smoke.cpp",
        .is_cpp = true,
        .objects = cpp_sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &cxx_inc,
        .extra_flags = &.{
            "-U_WIN32",
            "-U__MINGW32__",
            "-include", "picolibc_prereq.h",
            "-include", "__config_site",
        },
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, libcpp.step, picolibc_objs.step, xbox_objs.step, libcxx_objs.step },
    });
    const cpp23_step = b.step("conformance-cpp23", "Build C++23 conformance smokes");
    cpp23_step.dependOn(cpp23.install);

    const conformance_c = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "conformance-c",
        .src = "samples/conformance/c/main.c",
        .extra_srcs = &.{"samples/conformance/c/generated_tests.c"},
        .objects = sample_objects.items,
        .libs = &.{krnl},
        .include_paths = &.{ b.path("include"), b.path("build/generated"), b.path("src/runtime/c23"), b.path("vendor/picolibc/libc/include") },
        .entry = "start",
        .bootstrap = true,
        .deps = &.{ verify, &mkdir_samples.step, libc.step, picolibc_objs.step, xbox_objs.step },
    });
    const conformance_c_step = b.step("conformance-c", "Build C libc/runtime conformance runner (kit ISO)");
    conformance_c_step.dependOn(conformance_c.install);
}
