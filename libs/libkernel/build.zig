const std = @import("std");

pub const Result = struct {
    path: std.Build.LazyPath,
    step: *std.Build.Step,
};

// Build libkernel.lib: the Xbox kernel import library, generated from the
// checked-in decorated module-definition file (libs/libkernel/xboxkrnl.def).
//
// The .def is produced offline by tools/generate_xboxkrnl_lib.py from two
// authoritative leak artifacts -- the rebuilt console kernel map (decorated
// names + @N + calling convention) and the leak xboxkrnl.def (ordinals). The
// kernel export set is fixed, so the .def changes ~never; we commit it as the
// human-readable source of truth and let `zig lib` (LLVM, not MSVC) archive it.
//
// This replaces the opaque prebuilt/xboxkrnl.lib: the generated import surface
// is symbol-identical (371/371 __imp_ exports + descriptors) to that blob.
pub fn add(b: *std.Build, deps: []const *std.Build.Step) Result {
    const lib_path = "zig-out/lib/libkernel.lib";
    const def_path = "libs/libkernel/xboxkrnl.def";

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
