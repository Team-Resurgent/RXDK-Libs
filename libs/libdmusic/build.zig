const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const dmusic_sources = @import("sources.zig");

const X = "libs/libdmusic";

// Include environment for the Xbox DirectMusic runtime (dmusic.lib). Every
// component's TUs pull the DirectMusic interface headers (dmusicc/dmusics/
// dmusici/dmusicf, staged in inc/), the private headers (dmusicip.h, and the
// shared/ private surface dmusicp.h/dmusiccp.h/xsoundp.h/dmstrm.h ...), <xtl.h>
// and the PUBLIC <dsound.h> (DirectMusic outputs through DirectSound). Components
// cross-include by relative path (dmstyle -> ..\dmband\dmbandp.h), so every
// component dir is on the search path. site/bridge_dmusic.h (force-included)
// pre-pulls the xtl/dsound/COM/CRT umbrella. Title-side like libxact -- links
// alongside libdsound + libkernel + libxapi + libc.
pub fn includeDirs() []const []const u8 {
    return &.{
        X ++ "/inc",
        X ++ "/shared",
        X ++ "/xprivate",
        X ++ "/dmguids",
        X ++ "/dmsynth",
        X ++ "/dmloader",
        X ++ "/dmband",
        X ++ "/dmstyle",
        X ++ "/dmcompos",
        X ++ "/dmime",
        X ++ "/dmscript",
        X ++ "/dswave",
        X ++ "/site",
        "shared/include",
        "libs/libxapi/internal",
        "libs/libxapi/internal/shims",
        "libs/libxapi/support/inc",
        "libs/libxapi/support/inc/ntos",
        "libs/libxapi/nt",
        "libs/libxapi/rtl/inc",
        "libs/libxapi/port",
        "libs/libxapi/site",
        "build/generated",
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
    };
}

// Title-side flags, modeled on libxact. -fms-extensions/-fms-compatibility:
// __declspec/pragma/anonymous-struct MSVC-isms throughout DirectMusic.
// -fasm-blocks: dmsynth's mix kernels + muldiv keep MSVC __asm{}. -fno-operator-
// names: some headers use and/or/not as identifiers. The shared dmusic C_DEFINES
// (from the leak sources.inc): _WIN32 + XBOX select the xbox #ifdef paths,
// INC_OLE2/_MT/UNICODE the OLE/threaded/wide build, DSOUND_IFACE_VERSION=0x4000,
// DPF_LIBRARY/POOL_TAG the debug/pool tags. dmusver.h is force-included (leak
// /FIdmusver.h). The bridge supplies the Win32/COM/CRT env + forces retail.
const common_flags = [_][]const u8{
    "-ffreestanding",
    "-fno-stack-protector",
    "-fdata-sections",
    "-ffunction-sections",
    "-nostdinc",
    "-fms-extensions",
    "-fms-compatibility",
    "-fasm-blocks",
    "-fno-operator-names",
    "-fno-sanitize=undefined",
    "-fno-builtin",
    "-Wno-everything",
    "-D_WIN32",
    "-DXBOX",
    "-DINC_OLE2",
    "-D_MT",
    "-DUNICODE",
    "-D_UNICODE",
    "-DWIN32",
    "-DDSOUND_IFACE_VERSION=0x4000",
    "-DDPF_LIBRARY=\"DMUSIC\"",
    "-DPOOL_TAG='SUMD'",
    "-DASSUME_VALID_PARAMETERS",
    // Same reason as libdsound's DSOUND_NO_OVERRIDE_NEW_DELETE: shared/xalloc.h
    // and dmloader/opnew.cpp define the *global* operator new/delete, so a title
    // that links libdmusic (which a link reaches before libcpp.lib) got its whole
    // C++ allocator from DirectMusic. Worse than libdsound's case, the two
    // disagree -- xalloc.h routes at DirectMusicAllocI while opnew.cpp, whose
    // strong definitions win for the scalar forms, routes at malloc -- so `new`
    // and `new[]` came from different heaps. DirectMusicAllocI only promises
    // DWORD alignment (see the ASSERTMSG in dmime/debug.cpp), which is not enough
    // for the SSE loads in the XDK's math code. Let libcpp supply the operators;
    // DirectMusic's own pool stays reachable through DirectMusicAlloc/Free.
    "-DDMUSIC_NO_OVERRIDE_NEW_DELETE",
    "-include",
    "picolibc.h",
    "-include",
    "libs/libxapi/site/profile.h",
    "-include",
    X ++ "/shared/dmusver.h",
    "-include",
    X ++ "/site/bridge_dmusic.h",
};

fn sliceFlags(b: *std.Build, slice: dmusic_sources.Slice) []const []const u8 {
    var list = std.ArrayListUnmanaged([]const u8).empty;
    if (slice.is_cpp) {
        // DirectMusic genuinely uses C++ try/catch (defensive catch(...) around
        // throwing new, plus the scripting engine) -- compile with exceptions so
        // those sites parse. All catches in the ported set are catch(...), so no
        // RTTI is needed; the EH personality/__cxa_* land in libcpp at title link.
        list.appendSlice(b.allocator, &.{ "-std=c++17", "-fexceptions", "-fno-rtti", "-nostdinc++" }) catch @panic("OOM");
    } else {
        list.appendSlice(b.allocator, &.{"-std=c17"}) catch @panic("OOM");
    }
    list.appendSlice(b.allocator, &common_flags) catch @panic("OOM");
    list.appendSlice(b.allocator, slice.extra_defines) catch @panic("OOM");
    if (slice.pch) |pch| {
        list.appendSlice(b.allocator, &.{ "-include", pch }) catch @panic("OOM");
    }
    return list.toOwnedSlice(b.allocator) catch @panic("OOM");
}

// Flags for the dmsynth-seq slice: libdsound's DRIVER environment, mirroring
// libs/libdsound/build.zig's common set (it must match -- this TU compiles
// libdsound's internal headers). -DXMIX because the whole file is guarded on it;
// POOL_TAG/DPF_LIBRARY because it checks for both at the top.
fn driverEnvFlags(b: *std.Build) []const []const u8 {
    const base = [_][]const u8{
        "-std=c++17",       "-fexceptions",      "-fno-rtti",           "-nostdinc++",
        "-ffreestanding",   "-fno-stack-protector", "-fdata-sections",  "-ffunction-sections",
        "-nostdinc",        "-fms-extensions",   "-fms-compatibility",  "-fasm-blocks",
        "-fno-operator-names", "-fno-sanitize=undefined", "-fno-builtin", "-Wno-everything",
        "-D_XAPI_",         "-DDPF_LIBRARY=\"DSOUND\"", "-DXMIX",      "-DPOOL_TAG='SUMD'",
        // This TU sees both libdsound's memmgr.h and dmusic's xalloc.h, so it
        // needs both libraries' opt-outs or it alone would put the global
        // operators back into libdmusic.lib.
        "-DDSOUND_NO_OVERRIDE_NEW_DELETE", "-DDMUSIC_NO_OVERRIDE_NEW_DELETE",
        "-Xclang",          "-fdefault-calling-conv=stdcall",
        "-include",         "libs/libdsound/site/cdecl_libc.h",
        "-include",         "picolibc.h",
        "-include",         "libs/libxapi/site/profile.h",
        "-include",         "libs/libdsound/site/bridge_dsound.h",
    };
    return b.allocator.dupe([]const u8, &base) catch @panic("OOM");
}

// Include path for the dmsynth-seq slice: dmusic's own directories (for
// dsoundsequencer.h, dmime/cmixbins.h, xalloc.h) ahead of libdsound's internal
// tree, then the shared set.
fn driverEnvIncludeDirs() []const []const u8 {
    return &.{
        X ++ "/dmsynth",
        X ++ "/dmime",
        X ++ "/xprivate",
        X ++ "/shared",
        X ++ "/inc",
        "libs/libdsound/dsound",
        "libs/libdsound/common",
        "libs/libdsound/ac97",
        "libs/libdsound/tools_inc",
        "shared/include",
        "libs/libxapi/internal",
        "libs/libxapi/internal/shims",
        "libs/libxapi/support/inc",
        "libs/libxapi/support/inc/ntos",
        "libs/libxapi/nt",
        "libs/libxapi/rtl/inc",
        "libs/libxapi/port",
        "libs/libxapi/site",
        "build/generated",
        "shared/picolibc/include",
        "shared/picolibc/machine/x86",
    };
}

pub const ObjectBatch = struct {
    step: *std.Build.Step,
    outputs: []const std.Build.LazyPath,
};

pub fn addAllObjects(
    b: *std.Build,
    xbox_target: @TypeOf(@import("../../build/xbox_target.zig")),
    opt_flag: []const u8,
) ObjectBatch {
    const allocator = b.allocator;
    var all_outputs = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    var all_steps = std.ArrayListUnmanaged(*std.Build.Step).empty;

    for (dmusic_sources.slices) |slice| {
        const flags = if (slice.driver_env) driverEnvFlags(b) else sliceFlags(b, slice);
        const batch = compile_c.addBatch(b, .{
            .name = b.fmt("dmusic-{s}", .{slice.name}),
            .target = xbox_target.target_triple,
            .out_subdir = b.fmt("dmusic/{s}", .{slice.name}),
            .sources = slice.sources,
            .flags = flags,
            .include_dirs = if (slice.driver_env) driverEnvIncludeDirs() else includeDirs(),
            .opt_flag = opt_flag,
            .is_cpp = slice.is_cpp,
        });
        all_outputs.appendSlice(allocator, batch.outputs) catch @panic("OOM");
        all_steps.append(allocator, batch.step) catch @panic("OOM");
    }

    const aggregate = b.allocator.create(std.Build.Step) catch @panic("OOM");
    aggregate.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "compile-dmusic-all",
        .owner = b,
    });
    for (all_steps.items) |s| aggregate.dependOn(s);

    return .{
        .step = aggregate,
        .outputs = all_outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
