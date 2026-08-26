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
const libxfont_pkg = @import("libs/libxfont/build.zig");
const libxact_pkg = @import("libs/libxact/build.zig");
const libxonline_pkg = @import("libs/libxonline/build.zig");
const libxvoice_pkg = @import("libs/libxvoice/build.zig");
const libdmusic_pkg = @import("libs/libdmusic/build.zig");
const libkernel_pkg = @import("libs/libkernel/build.zig");
const libxbdm_pkg = @import("libs/libxbdm/build.zig");
const compile_c = @import("build/compile_c.zig");
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
    const optimize = b.standardOptimizeOption(.{});
    const opt_flag = optFlag(optimize);

    const verify = verify_no_vs.addStep(b);
    const verify_step = b.step("verify-no-vs", "Assert build/*.zig never invokes MSVC toolchain");
    verify_step.dependOn(verify);

    const mkdir_lib = b.addWriteFiles();
    _ = mkdir_lib.add("zig-out/lib/.keep", "");

    const picolibc_objs = picolibc.addPicolibcObjects(b, xbox_target, opt_flag) catch @panic("picolibc sources");
    const xbox_objs = picolibc.addXboxObjects(b, xbox_target, opt_flag);

    var libc_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    libc_objects.appendSlice(b.allocator, picolibc_objs.outputs) catch @panic("OOM");
    libc_objects.appendSlice(b.allocator, xbox_objs.outputs) catch @panic("OOM");
    // xbox_objs includes xbox/xbld.c, which emits the .XBLD / XboxKrnlBuildNumber
    // library-version record every title's XBE must carry (startup.c references it
    // to pull the object into libc.lib) -- so no loose/prebuilt object is needed.

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

    // Title-compiled XapiTitleStartup object: the XAPI title entry (XAPI+CRT+TLS
    // bring-up before main). Compiled with the *title* recipe (cFlags + -D_XAPI_/
    // -fms-*), never the internal libxapi recipe -- RXDK_LIBXAPI_BUILD would break
    // the ABI. Packed into libxapi.lib (and _core) so any title that links libxapi
    // resolves the entry from the archive (pulled by -e XapiTitleStartup); no loose
    // dist object, and no need to know at stage time whether libxapi is in use.
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

    const xapi_objs = libxapi_pkg.addAllObjects(b, xbox_target, opt_flag);
    var xapi_lib_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    xapi_lib_objects.appendSlice(b.allocator, xapi_objs.outputs) catch @panic("OOM");
    xapi_lib_objects.append(b.allocator, xapi_start_batch.outputs[0]) catch @panic("OOM");
    var xapi_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xapi_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xapi_deps.append(b.allocator, xapi_objs.step) catch @panic("OOM");
    xapi_deps.append(b.allocator, xapi_start_batch.step) catch @panic("OOM");
    const libxapi = coff_lib.pack(b, "libxapi", xapi_lib_objects.items, xapi_deps.items);
    const install_libxapi = b.addInstallFile(libxapi.path, "lib/libxapi.lib");
    install_libxapi.step.dependOn(libxapi.step);

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
    // In the default install: every graphical title links it, so leaving it out
    // means a plain `zig build` stages whatever libd3d8.lib was already there,
    // silently deploying an archive built from different sources or flags.
    b.getInstallStep().dependOn(&install_libd3d8.step);
    const d3d8_step = b.step("libd3d8", "Build libd3d8.lib (Xbox D3D8 / NV2A driver)");
    d3d8_step.dependOn(&install_libd3d8.step);

    // libd3d8i: the same driver with the performance instrumentation compiled in
    // (-DPROFILE=1), matching the XDK's d3d8i.lib. It exports the D3DPERF
    // statistics surface -- GetStatistics/Reset/GetPushBufferInfo/Start+
    // StopPerfProfile and the counters behind them -- which retail d3d8.lib does
    // not, because counting draw calls and state changes on every call is a cost
    // a shipping title should not pay. A title that profiles links this INSTEAD
    // of libd3d8; the event markers (BeginEvent/EndEvent/SetMarker) are in both.
    //
    // Not in the default install: it is opt-in, and building it doubles the d3d8
    // compile.
    const d3d8i_objs = libd3d8_pkg.addAllObjectsVariant(b, xbox_target, opt_flag, true);
    var d3d8i_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    d3d8i_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    d3d8i_deps.append(b.allocator, d3d8i_objs.step) catch @panic("OOM");
    const libd3d8i = coff_lib.pack(b, "libd3d8i", d3d8i_objs.outputs, d3d8i_deps.items);
    const install_libd3d8i = b.addInstallFile(libd3d8i.path, "lib/libd3d8i.lib");
    install_libd3d8i.step.dependOn(libd3d8i.step);
    const d3d8i_step = b.step("libd3d8i", "Build libd3d8i.lib (D3D8 with D3DPERF instrumentation)");
    d3d8i_step.dependOn(&install_libd3d8i.step);

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
    //
    // XFONT is packed in here too, because that is where the retail XDK keeps it
    // -- xgraphics.lib exports the whole XFONT_* set, there is no xfont.lib. Its
    // sources stay under libs/libxfont and build with their own flags (the
    // TrueType scan converter in particular needs an isolated environment), the
    // objects just land in this archive. Same shape as dsac97 folding into
    // libdsound.
    const xg_objs = libxgraphics_pkg.addAllObjects(b, xbox_target, opt_flag);
    const xfont_objs = libxfont_pkg.addAllObjects(b, xbox_target, opt_flag);
    var xg_objects = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    xg_objects.appendSlice(b.allocator, xg_objs.outputs) catch @panic("OOM");
    xg_objects.appendSlice(b.allocator, xfont_objs.outputs) catch @panic("OOM");
    var xg_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xg_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xg_deps.append(b.allocator, xg_objs.step) catch @panic("OOM");
    xg_deps.append(b.allocator, xfont_objs.step) catch @panic("OOM");
    const libxgraphics = coff_lib.pack(b, "libxgraphics", xg_objects.items, xg_deps.items);
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
    //
    // Two variants, as the retail XDK ships: this one is XNET_BUILD_LIBX, the plain
    // sockets stack, which links standalone. libxneto below is XNET_BUILD_LIBO, which
    // adds ONLINE/QOS/SG and needs libxonline for the secure-gateway Kerberos calls
    // its ip.cpp then contains. See libs/libxnet/site/bridge_xnet.h.
    const xnet_objs = libxnet_pkg.addAllObjectsVariant(b, xbox_target, opt_flag, false);
    var xnet_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xnet_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xnet_deps.append(b.allocator, xnet_objs.step) catch @panic("OOM");
    const libxnet = coff_lib.pack(b, "libxnet", xnet_objs.outputs, xnet_deps.items);
    const install_libxnet = b.addInstallFile(libxnet.path, "lib/libxnet.lib");
    install_libxnet.step.dependOn(libxnet.step);
    const xnet_step = b.step("libxnet", "Build libxnet.lib (Xbox XNet stack, sockets only)");
    xnet_step.dependOn(&install_libxnet.step);

    // libxneto: the same stack with the online feature set (XNET_FEATURE_ONLINE +
    // _QOS + _SG) -- the CXoBase/CXn secure-gateway logon layer libxonline's CXo
    // derives from. Pair it with libxonline.lib; on its own it leaves the four
    // CXoBase::XoKerb* calls in ip.cpp unresolved.
    const xneto_objs = libxnet_pkg.addAllObjectsVariant(b, xbox_target, opt_flag, true);
    var xneto_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xneto_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xneto_deps.append(b.allocator, xneto_objs.step) catch @panic("OOM");
    const libxneto = coff_lib.pack(b, "libxneto", xneto_objs.outputs, xneto_deps.items);
    const install_libxneto = b.addInstallFile(libxneto.path, "lib/libxneto.lib");
    install_libxneto.step.dependOn(libxneto.step);
    const xneto_step = b.step("libxneto", "Build libxneto.lib (XNet stack with Xbox Live/QoS/SG)");
    xneto_step.dependOn(&install_libxneto.step);

    // MSVC 64-bit divide/mod/mul helpers (__alldiv/__aulldiv/__allrem/__aullrem/
    // __allmul) that libxnet's MSVC-C++-ABI object code calls into (see
    // libs/libxapi/port/msvc_lldiv.c for the ABI-shim rationale). Compiled once
    // here (GNU ABI, so the C operators inside lower to compiler-rt) and packed
    // into libcompat.lib by build-iso.ps1 -Dist, so every consumer of libxnet
    // gets it for free instead of needing its own copy of this file.
    const msvc_lldiv_batch = compile_c.addBatch(b, .{
        .name = "msvc-lldiv",
        .target = xbox_target.target_triple,
        .out_subdir = "compat",
        .sources = &.{"libs/libxapi/port/msvc_lldiv.c"},
        .flags = &.{ "-std=c17", "-ffreestanding", "-fno-stack-protector", "-fno-sanitize=undefined" },
        .include_dirs = &.{},
        .opt_flag = opt_flag,
        .is_cpp = false,
    });
    xnet_step.dependOn(msvc_lldiv_batch.step);
    xneto_step.dependOn(msvc_lldiv_batch.step);

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

    // XFONT has no library of its own: its objects are packed into libxgraphics
    // above, matching the retail XDK. `zig build libxfont` is kept as an alias so
    // the name still works and so the font samples' build step is unchanged.
    const xfont_step = b.step("libxfont", "Build XFONT (packed into libxgraphics.lib, as in the retail XDK)");
    xfont_step.dependOn(&install_libxgraphics.step);

    // libxact: the Xbox XACT runtime audio engine (xacteng.lib), ported from the
    // leak (private/windows/directx/xact/runtime/engine). Title-side code, same
    // shape as libxmv -- drives DirectSound's public API + xboxkrnl timer/DPC/pool,
    // no direct APU/PCI pokes. Not in the default install (archives fine;
    // undefined externals -- XactMemAlloc/DwDbg* -- only surface at title link).
    const xact_objs = libxact_pkg.addAllObjects(b, xbox_target, opt_flag);
    var xact_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xact_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xact_deps.append(b.allocator, xact_objs.step) catch @panic("OOM");
    const libxact = coff_lib.pack(b, "libxact", xact_objs.outputs, xact_deps.items);
    const install_libxact = b.addInstallFile(libxact.path, "lib/libxact.lib");
    install_libxact.step.dependOn(libxact.step);
    const xact_step = b.step("libxact", "Build libxact.lib (Xbox XACT runtime audio engine)");
    xact_step.dependOn(&install_libxact.step);

    // libxonline: the Xbox Live client (xonline.lib), ported from the leak
    // (private/online). Title-side client on top of libxnet + libkernel + libxapi +
    // libc: logon/presence/accounts/billing/match/stats/users/service/msgclient,
    // content download+patching, the XDK's own crypto (Kerberos/MD5/ASN.1) and LZX
    // decompressor. Compiled with the MSVC C++ ABI (see libs/libxonline/build.zig).
    // Xbox Live servers are long dead -- compile + link only. Not in the default
    // install.
    const xonline_objs = libxonline_pkg.addAllObjects(b, xbox_target, opt_flag);
    var xonline_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xonline_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xonline_deps.append(b.allocator, xonline_objs.step) catch @panic("OOM");
    const libxonline = coff_lib.pack(b, "libxonline", xonline_objs.outputs, xonline_deps.items);
    const install_libxonline = b.addInstallFile(libxonline.path, "lib/libxonline.lib");
    install_libxonline.step.dependOn(libxonline.step);
    const xonline_step = b.step("libxonline", "Build libxonline.lib (Xbox Live client)");
    xonline_step.dependOn(&install_libxonline.step);

    // libuix: the UIX drop-in UI. 5849 ships this as its own uix.lib rather than
    // folding it into xonline.lib, and the audit in docs/5849-uplift.md showed our
    // libxonline carrying all 50 of its symbols. It calls into libxonline, so a
    // title links both -- libuix first.
    const uix_objs = libxonline_pkg.addUixObjects(b, xbox_target, opt_flag);
    var uix_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    uix_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    uix_deps.append(b.allocator, uix_objs.step) catch @panic("OOM");
    const libuix = coff_lib.pack(b, "libuix", uix_objs.outputs, uix_deps.items);
    const install_libuix = b.addInstallFile(libuix.path, "lib/libuix.lib");
    install_libuix.step.dependOn(libuix.step);
    const uix_step = b.step("libuix", "Build libuix.lib (the UIX drop-in Live UI)");
    uix_step.dependOn(&install_libuix.step);

    // libxvoice: the XDK-5849 voice library (xvoice.lib) -- the XHV high-level
    // voice-chat engine (xhv.h) + the low-level voice XMO/codec API (xvoice.h) +
    // the XDEVICE_TYPE_VOICE_* device tables. NOT a leak port (the leak has no
    // xvoice implementation): a fresh RXDK implementation of the 5849 public C
    // surface -- bookkeeping real, communicator/codec paths report no-headset
    // failure (see libs/libxvoice/sources.zig). Not in the default install.
    const xvoice_objs = libxvoice_pkg.addAllObjects(b, xbox_target, opt_flag);
    var xvoice_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    xvoice_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    xvoice_deps.append(b.allocator, xvoice_objs.step) catch @panic("OOM");
    const libxvoice = coff_lib.pack(b, "libxvoice", xvoice_objs.outputs, xvoice_deps.items);
    const install_libxvoice = b.addInstallFile(libxvoice.path, "lib/libxvoice.lib");
    install_libxvoice.step.dependOn(libxvoice.step);
    const xvoice_step = b.step("libxvoice", "Build libxvoice.lib (XDK-5849 XHV voice chat + low-level voice)");
    xvoice_step.dependOn(&install_libxvoice.step);

    // libdmusic: the Xbox DirectMusic runtime (dmusic.lib), ported from the leak
    // (private/windows/directx/dmusic). Title-side, COM-heavy interactive-music
    // stack -- software synthesizer (dmsynth) + performance/segment/track engine
    // (dmime) + loader + band/style/compos/script components -- that outputs audio
    // through libdsound's public API. Not in the default install (archives fine;
    // undefined externals only surface at title link).
    const dmusic_objs = libdmusic_pkg.addAllObjects(b, xbox_target, opt_flag);
    var dmusic_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    dmusic_deps.append(b.allocator, &mkdir_lib.step) catch @panic("OOM");
    dmusic_deps.append(b.allocator, dmusic_objs.step) catch @panic("OOM");
    const libdmusic = coff_lib.pack(b, "libdmusic", dmusic_objs.outputs, dmusic_deps.items);
    const install_libdmusic = b.addInstallFile(libdmusic.path, "lib/libdmusic.lib");
    install_libdmusic.step.dependOn(libdmusic.step);
    const dmusic_step = b.step("libdmusic", "Build libdmusic.lib (Xbox DirectMusic runtime)");
    dmusic_step.dependOn(&install_libdmusic.step);

    b.getInstallStep().dependOn(&install_libc.step);
    b.getInstallStep().dependOn(&install_libcpp.step);
    b.getInstallStep().dependOn(&install_libxapi.step);
    // The default build produces every shippable lib, not just the core set, so a
    // plain `zig build` matches what the dist packages (and no lib -- e.g. xonline
    // -- is silently skipped and left stale).
    b.getInstallStep().dependOn(&install_libd3d8i.step);
    b.getInstallStep().dependOn(&install_libd3dx8.step);
    b.getInstallStep().dependOn(&install_libxgraphics.step);
    b.getInstallStep().dependOn(&install_libdsound.step);
    b.getInstallStep().dependOn(&install_libxnet.step);
    b.getInstallStep().dependOn(&install_libxneto.step);
    b.getInstallStep().dependOn(&install_libxmv.step);
    b.getInstallStep().dependOn(&install_libxact.step);
    b.getInstallStep().dependOn(&install_libxonline.step);
    b.getInstallStep().dependOn(&install_libuix.step);
    b.getInstallStep().dependOn(&install_libxvoice.step);
    b.getInstallStep().dependOn(&install_libdmusic.step);
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


    // libxbdm: the Xbox debug-monitor (xbdm.dll) import library, generated from
    // the checked-in decorated .def (libs/libxbdm/xbdm.def) via `zig lib`. DXT
    // debug-monitor extensions link this for their Dm* imports (by ordinal),
    // alongside libkernel.lib for xboxkrnl.exe.
    const libxbdm = libxbdm_pkg.add(b, &.{&mkdir_lib.step});
    const install_libxbdm = b.addInstallFile(libxbdm.path, "lib/libxbdm.lib");
    install_libxbdm.step.dependOn(libxbdm.step);
    b.getInstallStep().dependOn(&install_libxbdm.step);
    const libxbdm_step = b.step("libxbdm", "Build libxbdm.lib (Xbox debug-monitor import library)");
    libxbdm_step.dependOn(&install_libxbdm.step);

}
