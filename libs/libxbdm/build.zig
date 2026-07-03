const std = @import("std");

pub const Result = struct {
    path: std.Build.LazyPath,
    step: *std.Build.Step,
};

// Build libxbdm.lib: the Xbox debug-monitor (xbdm.dll) import library, generated
// from the checked-in decorated module-definition file (libs/libxbdm/xbdm.def).
//
// The .def was extracted offline from the leak's public/xdk/lib/xbdm.lib
// short-import members (decorated __stdcall names + @ordinal, all NONAME), so it
// is symbol-identical to the retail xbdm.lib. xbdm's export set is fixed, so the
// .def changes ~never; we commit it as the human-readable source of truth and let
// `zig lib` (LLVM, not MSVC) archive it -- exactly like libkernel.
//
// DXT (debug-monitor extension) images link this to import Dm* APIs by ordinal
// from xbdm.dll, alongside libkernel.lib for their xboxkrnl.exe imports.
pub fn add(b: *std.Build, deps: []const *std.Build.Step) Result {
    const lib_path = "zig-out/lib/libxbdm.lib";
    const def_path = "libs/libxbdm/xbdm.def";

    const run = b.addSystemCommand(&.{ b.graph.zig_exe, "lib", "/NOLOGO", "/machine:x86" });
    run.addArg(b.fmt("/def:{s}", .{def_path}));
    run.addArg(b.fmt("/out:{s}", .{lib_path}));
    run.setCwd(b.path("."));
    for (deps) |dep| {
        run.step.dependOn(dep);
    }

    return .{
        .path = b.path(lib_path),
        .step = &run.step,
    };
}
