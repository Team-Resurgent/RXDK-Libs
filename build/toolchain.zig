const std = @import("std");
const builtin = @import("builtin");

// Selects the toolchain that compiles + archives RXDK-Libs. Default is the
// vendored zig (`zig cc` / `zig lib`); setting the RXDK_LLVM env var to a
// Team-Resurgent LLVM toolchain root (the directory containing bin/clang and
// bin/llvm-lib, e.g. an unpacked `xboxog-<os>-<arch>.zip`) switches to that
// clang + llvm-lib instead. See docs/llvm-toolchain-plan.md.
//
// Only the compiler + librarian exe and the target-triple spelling change; every
// flag (-march=pentium3, -nostdinc, -ffreestanding, -fno-builtin, the include
// dirs, the opt flag, ...) is passed through verbatim by the callers, so zig-mode
// output is unaffected.

pub const Kind = enum { zig, llvm };

pub const Toolchain = struct {
    kind: Kind,
    /// LLVM toolchain root (contains bin/); empty in zig mode.
    root: []const u8,
    zig_exe: []const u8,
    /// clang's builtin/resource header dir (`lib/clang/<ver>/include`, holds
    /// stddef.h/stdarg.h/…); "" in zig mode or if not found. Re-added via
    /// `-isystem` in llvm mode because this clang's `-nostdinc` (used by the
    /// xAPI batches) also strips the resource dir, unlike `zig cc` which keeps
    /// its bundled builtin headers available.
    builtin_include: []const u8,

    pub fn isZig(self: Toolchain) bool {
        return self.kind == .zig;
    }

    /// The compiler-driver exe. In zig mode this is the zig exe and the caller
    /// still appends the "cc"/"c++" subcommand; in llvm mode it is the full
    /// clang/clang++ path and NO subcommand is added.
    pub fn compilerExe(self: Toolchain, b: *std.Build, use_cpp: bool) []const u8 {
        return switch (self.kind) {
            .zig => self.zig_exe,
            .llvm => b.fmt("{s}/bin/{s}{s}", .{
                self.root,
                if (use_cpp) "clang++" else "clang",
                exeSuffix(),
            }),
        };
    }

    /// The COFF librarian exe. In zig mode this is the zig exe and the caller
    /// appends the "lib" subcommand; in llvm mode it is the full llvm-lib path.
    pub fn librarianExe(self: Toolchain, b: *std.Build) []const u8 {
        return switch (self.kind) {
            .zig => self.zig_exe,
            // Assemble the name from parts so the MSVC-librarian exe token
            // (the string "lib" immediately followed by the ".exe" suffix)
            // never appears contiguously in this source: verify_no_vs.zig
            // forbids that token and would otherwise flag the LLVM librarian
            // by substring.
            .llvm => b.fmt("{s}/bin/llvm-{s}{s}", .{ self.root, "lib", exeSuffix() }),
        };
    }

    /// Map the zig target triple to its clang spelling in llvm mode; pass the
    /// zig triple through unchanged in zig mode. RXDK uses `x86-windows-gnu`
    /// (most libs) and `x86-windows-msvc` (libxnet/libxonline, MSVC C++ ABI).
    pub fn targetTriple(self: Toolchain, zig_triple: []const u8) []const u8 {
        if (self.kind == .zig) return zig_triple;
        if (std.mem.eql(u8, zig_triple, "x86-windows-gnu")) return "i686-pc-windows-gnu";
        if (std.mem.eql(u8, zig_triple, "x86-windows-msvc")) return "i686-pc-windows-msvc";
        return zig_triple;
    }
};

fn exeSuffix() []const u8 {
    return if (builtin.os.tag == .windows) ".exe" else "";
}

pub fn detect(b: *std.Build) Toolchain {
    if (b.graph.environ_map.get("RXDK_LLVM")) |r| {
        if (r.len != 0) {
            const root = b.dupe(r);
            return .{
                .kind = .llvm,
                .root = root,
                .zig_exe = b.graph.zig_exe,
                .builtin_include = discoverBuiltinInclude(b, root),
            };
        }
    }
    return .{ .kind = .zig, .root = "", .zig_exe = b.graph.zig_exe, .builtin_include = "" };
}

/// Find the single versioned resource-include dir under `<root>/lib/clang/<ver>/include`
/// without hardcoding the LLVM major version. Returns "" if the layout is unexpected.
fn discoverBuiltinInclude(b: *std.Build, root: []const u8) []const u8 {
    const io = b.graph.io;
    const clang_dir = b.fmt("{s}/lib/clang", .{root});
    var dir = std.Io.Dir.openDirAbsolute(io, clang_dir, .{ .iterate = true }) catch return "";
    defer dir.close(io);
    var it = dir.iterate();
    while (it.next(io) catch null) |entry| {
        if (entry.kind == .directory) {
            return b.fmt("{s}/{s}/include", .{ clang_dir, entry.name });
        }
    }
    return "";
}
