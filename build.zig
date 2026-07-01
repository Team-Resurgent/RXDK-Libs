const std = @import("std");

const xbox_target = @import("build/xbox_target.zig");
const coff_lib = @import("build/coff_lib.zig");
const picolibc = @import("libs/libc/build.zig");
const libcxx = @import("libs/libcpp/build.zig");
const libunwind = @import("libs/libcpp/unwind.zig");
const libxapi_pkg = @import("libs/libxapi/build.zig");
const libd3d8_pkg = @import("libs/libd3d8/build.zig");
const libd3dx8_pkg = @import("libs/libd3dx8/build.zig");
const libxgraphics_pkg = @import("libs/libxgraphics/build.zig");
const libdsound_pkg = @import("libs/libdsound/build.zig");
const libxnet_pkg = @import("libs/libxnet/build.zig");
const libxmv_pkg = @import("libs/libxmv/build.zig");
const libkernel_pkg = @import("libs/libkernel/build.zig");
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

    // libd3d8: the Xbox D3D8 (NV2A) driver. Same pattern as libxapi.
    const d3d8_objs = libd3d8_pkg.addAllObjects(b, xbox_target, opt_flag);
    var d3d8_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    d3d8_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    d3d8_deps.append(b.allocator, d3d8_objs.step) catch @panic("OOM");
    const libd3d8 = coff_lib.pack(b, "libd3d8", d3d8_objs.outputs, d3d8_deps.items);
    const install_libd3d8 = b.addInstallFile(libd3d8.path, "lib/libd3d8.lib");
    install_libd3d8.step.dependOn(libd3d8.step);
    const d3d8_step = b.step("libd3d8", "Build libd3d8.lib (Xbox D3D8 / NV2A driver)");
    d3d8_step.dependOn(&install_libd3d8.step);

    // libd3dx8: the Xbox D3DX8 helper library (math/mesh/tex/effect/.X + jpeg/png/zlib).
    // Title-side code; same pack pattern as libd3d8. Not in the default install
    // (archives fine; undefined externals only surface at title link-time).
    const d3dx8_objs = libd3dx8_pkg.addAllObjects(b, xbox_target, opt_flag);
    var d3dx8_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    d3dx8_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    d3dx8_deps.append(b.allocator, d3dx8_objs.step) catch @panic("OOM");
    const libd3dx8 = coff_lib.pack(b, "libd3dx8", d3dx8_objs.outputs, d3dx8_deps.items);
    const install_libd3dx8 = b.addInstallFile(libd3dx8.path, "lib/libd3dx8.lib");
    install_libd3dx8.step.dependOn(libd3dx8.step);
    const d3dx8_step = b.step("libd3dx8", "Build libd3dx8.lib (Xbox D3DX8 helper library)");
    d3dx8_step.dependOn(&install_libd3dx8.step);

    // libxgraphics: the Xbox xgraphics helper library (swizzle/format helpers,
    // S3TC, shader assembler, xgmath). d3dx8's texture path needs the XGSwizzle*
    // functions. Built >= -O2 (asm-blocks). Not in the default install.
    const xg_objs = libxgraphics_pkg.addAllObjects(b, xbox_target, opt_flag);
    var xg_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xg_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xg_deps.append(b.allocator, xg_objs.step) catch @panic("OOM");
    const libxgraphics = coff_lib.pack(b, "libxgraphics", xg_objs.outputs, xg_deps.items);
    const install_libxgraphics = b.addInstallFile(libxgraphics.path, "lib/libxgraphics.lib");
    install_libxgraphics.step.dependOn(libxgraphics.step);
    const xg_step = b.step("libxgraphics", "Build libxgraphics.lib (Xbox xgraphics helper library)");
    xg_step.dependOn(&install_libxgraphics.step);

    // libdsound: the Xbox core DirectSound library (MCPX APU driver). Not in the
    // default install.
    const ds_objs = libdsound_pkg.addAllObjects(b, xbox_target, opt_flag);
    var ds_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    ds_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    ds_deps.append(b.allocator, ds_objs.step) catch @panic("OOM");
    const libdsound = coff_lib.pack(b, "libdsound", ds_objs.outputs, ds_deps.items);
    const install_libdsound = b.addInstallFile(libdsound.path, "lib/libdsound.lib");
    install_libdsound.step.dependOn(libdsound.step);
    const ds_step = b.step("libdsound", "Build libdsound.lib (Xbox DirectSound / MCPX APU)");
    ds_step.dependOn(&install_libdsound.step);

    // libxnet: the Xbox XNet stack (private/ntos/net -- public XNetStartup API,
    // native CXbdmServer/CXbdmClient dev-kit NIC sharing, DHCP/DNS/ARP/ICMP/TCP/UDP).
    // Compiled with the MSVC C++ ABI (see libs/libxnet/build.zig). Not in the default
    // install.
    const xnet_objs = libxnet_pkg.addAllObjects(b, xbox_target, opt_flag);
    var xnet_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xnet_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xnet_deps.append(b.allocator, xnet_objs.step) catch @panic("OOM");
    const libxnet = coff_lib.pack(b, "libxnet", xnet_objs.outputs, xnet_deps.items);
    const install_libxnet = b.addInstallFile(libxnet.path, "lib/libxnet.lib");
    install_libxnet.step.dependOn(libxnet.step);
    const xnet_step = b.step("libxnet", "Build libxnet.lib (Xbox XNet stack, XNetStartup)");
    xnet_step.dependOn(&install_libxnet.step);

    // libxmv: the Xbox XMV (FMV) video decoder ported from the leak
    // (private/windows/xmv/decoder). Title-side software codec -> YUY2 D3D surface;
    // links with libd3d8 + libdsound + libxapi. Not in the default install.
    const xmv_objs = libxmv_pkg.addAllObjects(b, xbox_target, opt_flag);
    var xmv_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xmv_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xmv_deps.append(b.allocator, xmv_objs.step) catch @panic("OOM");
    const libxmv = coff_lib.pack(b, "libxmv", xmv_objs.outputs, xmv_deps.items);
    const install_libxmv = b.addInstallFile(libxmv.path, "lib/libxmv.lib");
    install_libxmv.step.dependOn(libxmv.step);
    const xmv_step = b.step("libxmv", "Build libxmv.lib (Xbox XMV FMV decoder)");
    xmv_step.dependOn(&install_libxmv.step);

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

    // libkernel: the Xbox kernel import library, generated from the checked-in
    // decorated .def (libs/libkernel/xboxkrnl.def) via `zig lib`. Replaces the
    // opaque prebuilt/xboxkrnl.lib (symbol-identical import surface).
    const libkernel = libkernel_pkg.add(b, &.{&mkdir_lib.step});
    const install_libkernel = b.addInstallFile(libkernel.path, "lib/libkernel.lib");
    install_libkernel.step.dependOn(libkernel.step);
    b.getInstallStep().dependOn(&install_libkernel.step);
    const libkernel_step = b.step("libkernel", "Build libkernel.lib (Xbox kernel import library)");
    libkernel_step.dependOn(&install_libkernel.step);

    const krnl = libkernel.path;

    var sample_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    sample_objects.appendSlice(b.allocator, picolibc_objs.outputs) catch @panic("OOM");
    sample_objects.appendSlice(b.allocator, xbox_objs.outputs) catch @panic("OOM");

    const libxapi_lib = b.path("zig-out/lib/libxapi.lib");
    const libd3d8_lib = b.path("zig-out/lib/libd3d8.lib");
    const libd3dx8_lib = b.path("zig-out/lib/libd3dx8.lib");
    const libxgraphics_lib = b.path("zig-out/lib/libxgraphics.lib");
    const libdsound_lib = b.path("zig-out/lib/libdsound.lib");
    const libxmv_lib = b.path("zig-out/lib/libxmv.lib");
    const libxnet_lib = b.path("zig-out/lib/libxnet.lib");
    const xapi_inc = [_]std.Build.LazyPath{
        b.path("shared/include"),
        b.path("libs/libxapi/internal"),
        b.path("build/generated"),
        b.path("shared/picolibc/include"),
        b.path("shared/picolibc/machine/x86"),
    };

    // Standalone title-compiled XapiTitleStartup object, shipped in the dist as
    // xapi_start.obj so an external builder (RXDK-VSCode) links it without having
    // to recompile the startup against the flattened dist headers. Compiled with
    // the *title* recipe (cFlags + -D_XAPI_/-fms-*), never the internal libxapi
    // recipe -- that is the whole point (RXDK_LIBXAPI_BUILD would break the ABI).
    const xapi_start_batch = compile_c.addBatch(b, .{
        .name = "xapi-start",
        .target = xbox_target.target_triple,
        .out_subdir = "startup",
        .sources = &.{"libs/libxapi/dll/xapi_start.c"},
        .flags = xbox_target.appendFlags(b, xbox_target.cFlags(b), &.{
            "-D_XAPI_", "-fms-extensions", "-fms-compatibility",
            "-include", "build/generated/picolibc.h",
        }),
        .include_dirs = &.{
            "shared/include", "libs/libxapi/internal", "build/generated",
            "shared/picolibc/include", "shared/picolibc/machine/x86",
        },
        .opt_flag = opt_flag,
        .is_cpp = false,
    });
    const install_xapi_start = b.addInstallFile(xapi_start_batch.outputs[0], "lib/xapi_start.obj");
    install_xapi_start.step.dependOn(xapi_start_batch.step);
    b.getInstallStep().dependOn(&install_xapi_start.step);

    const xapi_smoke_extra = [_][]const u8{
        "libs/libxapi/dll/xapi_start.c",
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
        "samples/xapi-smoke/src/test_soundtrack.c",
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
        .entry = "XapiTitleStartup",
        .deps = &.{ verify, &mkdir_samples.step, libkernel.step, libc.step, libxapi.step, picolibc_objs.step, xbox_objs.step },
    });
    const xapi_smoke_step = b.step("xapi-smoke", "Build xAPI category smoke tests (27 tests, kit hardware)");
    xapi_smoke_step.dependOn(xapi_smoke.install);

    // xAPI input-device monitor. Reuses xapi-smoke's trace helpers (common.c);
    // the title startup now comes from libxapi (XapiTitleStartup), so the only
    // new file is its own main.c.
    const xapi_input_inc = [_]std.Build.LazyPath{
        b.path("samples/xapi-smoke/src"), // common.h
        b.path("shared/include"),
        b.path("libs/libxapi/internal"),
        b.path("build/generated"),
        b.path("shared/picolibc/include"),
        b.path("shared/picolibc/machine/x86"),
    };
    const xapi_input_extra = [_][]const u8{
        "libs/libxapi/dll/xapi_start.c",
        "samples/xapi-smoke/src/common.c",
    };
    const xapi_input = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "xapi-input",
        .src = "samples/xapi-input/src/main.c",
        .extra_srcs = &xapi_input_extra,
        .objects = sample_objects.items,
        .libs = &.{ libxapi_lib, krnl },
        .include_paths = &xapi_input_inc,
        .extra_flags = &.{
            "-D_XAPI_",
            "-fms-extensions",
            "-fms-compatibility",
            "-include",
            "build/generated/picolibc.h",
        },
        .entry = "XapiTitleStartup",
        .deps = &.{ verify, &mkdir_samples.step, libkernel.step, libc.step, libxapi.step, picolibc_objs.step, xbox_objs.step },
    });
    const xapi_input_step = b.step("xapi-input", "Build xAPI input device monitor (controller/mouse/IR/keyboard)");
    xapi_input_step.dependOn(xapi_input.install);

    // D3D8 rotating-triangle sample. C title: reuses the xapi boot/trace helpers,
    // includes the public d3d8.h, and links libd3d8.lib (+ libxapi + krnl).
    const d3d8_tri_inc = [_]std.Build.LazyPath{
        b.path("samples/xapi-smoke/src"), // common.h
        b.path("shared/include"),
        b.path("libs/libxapi/internal"),
        b.path("libs/libxapi/nt"), // guiddef.h (GUID/REFGUID for d3d8.h)
        b.path("build/generated"),
        b.path("shared/picolibc/include"),
        b.path("shared/picolibc/machine/x86"),
    };
    const d3d8_tri_extra = [_][]const u8{
        "libs/libxapi/dll/xapi_start.c",
        "samples/xapi-smoke/src/common.c",
    };
    const d3d8_tri = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "d3d8-triangle",
        .src = "samples/d3d8-triangle/src/main.c",
        .extra_srcs = &d3d8_tri_extra,
        .objects = sample_objects.items,
        .libs = &.{ libd3dx8_lib, libd3d8_lib, libxapi_lib, krnl },
        .include_paths = &d3d8_tri_inc,
        // d3d8.h pulls a heavy Win32 header set; like the libraries, force our
        // headers only (-nostdinc) so zig's MinGW winnt.h/basetsd.h/vadefs.h
        // don't get pulled in and clash with our xboxkrnl/windef definitions.
        .extra_flags = &.{
            "-D_XAPI_",
            "-fms-extensions",
            "-fms-compatibility",
            "-nostdinc",
            "-include",
            "picolibc.h",
        },
        .entry = "XapiTitleStartup",
        .deps = &.{ verify, &mkdir_samples.step, libkernel.step, libc.step, libxapi.step, &install_libd3d8.step, &install_libd3dx8.step, picolibc_objs.step, xbox_objs.step },
    });
    const d3d8_tri_step = b.step("d3d8-triangle", "Build the D3D8 rotating-triangle sample");
    d3d8_tri_step.dependOn(d3d8_tri.install);

    // D3D8 texture-grid sample. C title: loads the media images via libd3dx8's
    // D3DXCreateTextureFromFile and draws a 3x2 grid of drifting textured quads.
    // Links libd3dx8 + libxgraphics (swizzle) + libd3d8 + libxapi + krnl.
    const d3d8_tex_extra = [_][]const u8{
        "libs/libxapi/dll/xapi_start.c",
        "samples/xapi-smoke/src/common.c",
    };
    // libd3dx8's C++ image codecs use global operator new/delete; this C title has
    // no C++ runtime, so compile a tiny malloc-backed set into its own object.
    const opnew_batch = compile_c.addBatch(b, .{
        .name = "d3d8-textures-opnew",
        .target = xbox_target.target_triple,
        .out_subdir = "d3d8-textures",
        .sources = &.{"samples/d3d8-textures/src/opnew.cpp"},
        .flags = &.{
            "-std=c++17", "-ffreestanding", "-fno-exceptions", "-fno-rtti",
            "-nostdinc", "-nostdinc++", "-fms-extensions", "-fms-compatibility",
            "-fno-sanitize=undefined", "-Wno-everything", "-D_XAPI_", "-include", "picolibc.h",
        },
        .include_dirs = &.{
            "shared/include", "build/generated",
            "shared/picolibc/include", "shared/picolibc/machine/x86",
        },
        .opt_flag = opt_flag,
        .is_cpp = true,
    });
    var d3d8_tex_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    d3d8_tex_objects.appendSlice(b.allocator, sample_objects.items) catch @panic("OOM");
    d3d8_tex_objects.appendSlice(b.allocator, opnew_batch.outputs) catch @panic("OOM");
    const d3d8_tex = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "d3d8-textures",
        .src = "samples/d3d8-textures/src/main.c",
        .extra_srcs = &d3d8_tex_extra,
        .objects = d3d8_tex_objects.items,
        .libs = &.{ libd3dx8_lib, libxgraphics_lib, libd3d8_lib, libxapi_lib, krnl },
        .include_paths = &d3d8_tri_inc,
        .extra_flags = &.{
            "-D_XAPI_",
            "-fms-extensions",
            "-fms-compatibility",
            "-nostdinc",
            "-include",
            "picolibc.h",
        },
        .entry = "XapiTitleStartup",
        .deps = &.{ verify, &mkdir_samples.step, libkernel.step, libc.step, libxapi.step, &install_libd3d8.step, &install_libd3dx8.step, &install_libxgraphics.step, opnew_batch.step, picolibc_objs.step, xbox_objs.step },
    });
    const d3d8_tex_step = b.step("d3d8-textures", "Build the D3D8 texture-grid sample");
    d3d8_tex_step.dependOn(d3d8_tex.install);

    // XMV video-playback sample. C title: opens D:\media\test.xmv, decodes it with
    // libxmv (the leak FMV codec) into a YUY2 surface and shows it via the D3D8
    // overlay; brings up libdsound (MCPX APU) for the audio path. Links libxmv +
    // libd3d8 + libdsound (+ libxapi + krnl). Reuses the d3d8 include/extra sets.
    const xmv_sample = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "xmv-play",
        .src = "samples/xmv-play/src/main.c",
        .extra_srcs = &d3d8_tri_extra,
        .objects = sample_objects.items,
        .libs = &.{ libxmv_lib, libd3d8_lib, libdsound_lib, libxapi_lib, krnl },
        .include_paths = &d3d8_tri_inc,
        .extra_flags = &.{
            "-D_XAPI_",
            "-fms-extensions",
            "-fms-compatibility",
            "-nostdinc",
            "-include",
            "picolibc.h",
        },
        .entry = "XapiTitleStartup",
        .deps = &.{ verify, &mkdir_samples.step, libkernel.step, libc.step, libxapi.step, &install_libxmv.step, &install_libd3d8.step, &install_libdsound.step, picolibc_objs.step, xbox_objs.step },
    });
    const xmv_sample_step = b.step("xmv-play", "Build the XMV video-playback sample");
    xmv_sample_step.dependOn(xmv_sample.install);

    // DirectSound OGG-music sample. C title: decodes media\music.ogg with
    // stb_vorbis and plays it on a looping libdsound buffer (the MCPX APU).
    const dsmusic_inc = [_]std.Build.LazyPath{
        b.path("samples/xapi-smoke/src"), // common.h
        b.path("shared/include"),         // dsound.h
        b.path("libs/libxapi/internal"),
        b.path("libs/libxapi/nt"),        // guiddef.h
        b.path("vendor/stb"),             // stb_vorbis.c (via stb_vorbis_impl.c)
        b.path("build/generated"),
        b.path("shared/picolibc/include"),
        b.path("shared/picolibc/machine/x86"),
    };
    const dsmusic_extra = [_][]const u8{
        "libs/libxapi/dll/xapi_start.c",
        "samples/xapi-smoke/src/common.c",
        "samples/dsound-music/src/stb_vorbis_impl.c",
    };
    const dsmusic = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "dsound-music",
        .src = "samples/dsound-music/src/main.c",
        .extra_srcs = &dsmusic_extra,
        .objects = sample_objects.items,
        .libs = &.{ libdsound_lib, libxapi_lib, krnl },
        .include_paths = &dsmusic_inc,
        .extra_flags = &.{
            "-D_XAPI_",
            "-fms-extensions",
            "-fms-compatibility",
            "-nostdinc",
            "-include",
            "picolibc.h",
        },
        .entry = "XapiTitleStartup",
        .deps = &.{ verify, &mkdir_samples.step, libkernel.step, libc.step, libxapi.step, &install_libdsound.step, picolibc_objs.step, xbox_objs.step },
    });
    const dsmusic_step = b.step("dsound-music", "Build the DirectSound OGG-music sample");
    dsmusic_step.dependOn(dsmusic.install);

    // XNet bring-up sample: start the TCP/IP stack (MCPX NIC), wait for DHCP,
    // print the link state + leased IP. C title: reuses the xapi boot/trace
    // helpers, links libxnet (+ libxapi + krnl).
    const xnet_inc = [_]std.Build.LazyPath{
        b.path("samples/xapi-smoke/src"), // common.h
        b.path("shared/include"),
        b.path("libs/libxapi/internal"),
        b.path("libs/libxapi/nt"),
        b.path("build/generated"),
        b.path("shared/picolibc/include"),
        b.path("shared/picolibc/machine/x86"),
    };
    const xnet_extra = [_][]const u8{
        "libs/libxapi/dll/xapi_start.c",
        "samples/xapi-smoke/src/common.c",
        // MSVC 64-bit math helpers (__alldiv etc.) for the MSVC-ABI libxnet.
        // Linked directly into the sample (GNU ABI -> lowers to compiler-rt).
        "libs/libxapi/port/msvc_lldiv.c",
    };
    const xnet_sample = link_pe.addPeSample(b, target, optimize, xbox_target, .{
        .name = "xnet-net",
        .src = "samples/xnet-net/src/main.c",
        .extra_srcs = &xnet_extra,
        .objects = sample_objects.items,
        .libs = &.{ libxnet_lib, libxapi_lib, krnl },
        .include_paths = &xnet_inc,
        .extra_flags = &.{
            "-D_XAPI_",
            "-fms-extensions",
            "-fms-compatibility",
            "-nostdinc",
            "-include",
            "picolibc.h",
        },
        .entry = "XapiTitleStartup",
        .deps = &.{ verify, &mkdir_samples.step, libkernel.step, libc.step, libxapi.step, &install_libxnet.step, picolibc_objs.step, xbox_objs.step },
    });
    const xnet_sample_step = b.step("xnet-net", "Build the XNet network bring-up sample");
    xnet_sample_step.dependOn(xnet_sample.install);

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
        .deps = &.{ verify, &mkdir_samples.step, libkernel.step, libc.step, picolibc_objs.step, xbox_objs.step },
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
        .eh_frame_bracket = true,
        .deps = &.{ verify, &mkdir_samples.step, libkernel.step, libc.step, libcpp.step, picolibc_objs.step, xbox_objs.step, libcxx_objs.step, libunwind_objs.step },
    });
    const libcpp_smoke_step = b.step("libcpp-smoke", "Build libc++ / C++23 runtime smoke (kit ISO)");
    libcpp_smoke_step.dependOn(libcpp_smoke.install);
}
