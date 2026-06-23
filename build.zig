const std = @import("std");

const xbox_target = @import("build/xbox_target.zig");
const coff_lib = @import("build/coff_lib.zig");
const picolibc = @import("build/picolibc.zig");
const libcxx = @import("build/libcxx.zig");
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

    var libxboxc_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    libxboxc_objects.appendSlice(b.allocator, picolibc_objs.outputs) catch @panic("OOM");
    libxboxc_objects.appendSlice(b.allocator, xbox_objs.outputs) catch @panic("OOM");

    var libxboxc_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    libxboxc_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    libxboxc_deps.append(b.allocator, picolibc_objs.step) catch @panic("OOM");
    libxboxc_deps.append(b.allocator, xbox_objs.step) catch @panic("OOM");

    const libxboxc = coff_lib.pack(b, "libxboxc", libxboxc_objects.items, libxboxc_deps.items);
    const install_libxboxc = b.addInstallFile(libxboxc.path, "lib/libxboxc.lib");
    install_libxboxc.step.dependOn(libxboxc.step);

    const stage_c_headers = picolibc.stageHeaders(b);
    stage_c_headers.dependOn(&install_libxboxc.step);

    const libcxx_objs = libcxx.addLibcxxObjects(b, xbox_target, opt_flag) catch @panic("libcxx sources");

    var libxboxcxx_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    libxboxcxx_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    libxboxcxx_deps.append(b.allocator, libcxx_objs.step) catch @panic("OOM");
    libxboxcxx_deps.append(b.allocator, libxboxc.step) catch @panic("OOM");
    libxboxcxx_deps.append(b.allocator, xbox_objs.step) catch @panic("OOM");

    const libxboxcxx = coff_lib.pack(b, "libxboxcxx", libcxx_objs.outputs, libxboxcxx_deps.items);
    const install_libxboxcxx = b.addInstallFile(libxboxcxx.path, "lib/libxboxcxx.lib");
    install_libxboxcxx.step.dependOn(libxboxcxx.step);

    const stage_cxx_headers = libcxx.stageHeaders(b);
    stage_cxx_headers.dependOn(&install_libxboxcxx.step);

    b.getInstallStep().dependOn(&install_libxboxc.step);
    b.getInstallStep().dependOn(&install_libxboxcxx.step);
    b.getInstallStep().dependOn(stage_c_headers);
    b.getInstallStep().dependOn(stage_cxx_headers);
    b.getInstallStep().dependOn(verify);

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
        .deps = &.{ verify, &mkdir_samples.step, libxboxc.step, picolibc_objs.step, xbox_objs.step },
    });
    const kernel_api_step = b.step("kernel-api-smoke", "Build KeQuerySystemTime + stdio smoke PE");
    kernel_api_step.dependOn(kernel_api.install);

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
        .deps = &.{ verify, &mkdir_samples.step, libxboxc.step, picolibc_objs.step, xbox_objs.step },
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
        .deps = &.{ verify, &mkdir_samples.step, libxboxc.step, libxboxcxx.step, picolibc_objs.step, xbox_objs.step, libcxx_objs.step },
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
        .deps = &.{ verify, &mkdir_samples.step, libxboxc.step, picolibc_objs.step, xbox_objs.step },
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
        .deps = &.{ verify, &mkdir_samples.step, libxboxc.step, libxboxcxx.step, picolibc_objs.step, xbox_objs.step, libcxx_objs.step },
    });
    const cpp23_step = b.step("conformance-cpp23", "Build C++23 conformance smokes");
    cpp23_step.dependOn(cpp23.install);
}
