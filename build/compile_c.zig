const std = @import("std");
const toolchain = @import("toolchain.zig");

pub const CompileBatch = struct {
    step: *std.Build.Step,
    outputs: []const std.Build.LazyPath,
};

pub const Options = struct {
    name: []const u8,
    target: []const u8,
    out_subdir: []const u8,
    sources: []const []const u8,
    flags: []const []const u8,
    include_dirs: []const []const u8,
    opt_flag: []const u8,
    is_cpp: bool = false,
};

const MkdirContext = struct {
    step: std.Build.Step,
    b: *std.Build,
    out_dir: []const u8,
};

fn addObjDir(b: *std.Build, out_dir: []const u8) *std.Build.Step {
    const ctx = b.allocator.create(MkdirContext) catch @panic("OOM");
    ctx.* = .{
        .step = std.Build.Step.init(.{
            .id = .custom,
            .name = b.fmt("mkdir-{s}", .{out_dir}),
            .owner = b,
        }),
        .b = b,
        .out_dir = out_dir,
    };
    ctx.step.makeFn = makeObjDir;
    return &ctx.step;
}

fn makeObjDir(step: *std.Build.Step, options: std.Build.Step.MakeOptions) !void {
    _ = options;
    const ctx: *MkdirContext = @fieldParentPtr("step", step);
    try ctx.b.build_root.handle.createDirPath(ctx.b.graph.io, ctx.out_dir);
}

fn uniqueStem(b: *std.Build, allocator: std.mem.Allocator, src: []const u8) []const u8 {
    var sanitized = std.ArrayListUnmanaged(u8).empty;
    defer sanitized.deinit(allocator);
    for (src) |c| {
        const ch: u8 = switch (c) {
            '/', '\\' => '_',
            '.', ':', ' ' => '_',
            else => c,
        };
        sanitized.append(allocator, ch) catch @panic("OOM");
    }
    return b.dupe(sanitized.items);
}

pub fn addBatch(b: *std.Build, opts: Options) CompileBatch {
    const allocator = b.allocator;
    const tc = toolchain.detect(b);
    const target = tc.targetTriple(opts.target);
    const out_dir = b.fmt("zig-out/obj/{s}", .{opts.out_subdir});

    const mkdir = addObjDir(b, out_dir);

    var outputs = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    var steps = std.ArrayListUnmanaged(*std.Build.Step).empty;

    steps.append(allocator, mkdir) catch @panic("OOM");

    for (opts.sources) |src| {
        const ext = std.fs.path.extension(src);
        if (ext.len != 0 and std.ascii.eqlIgnoreCase(ext, ".asm")) continue;

        const stem = uniqueStem(b, allocator, src);
        const obj_rel = b.fmt("{s}/{s}.o", .{ out_dir, stem });
        outputs.append(allocator, b.path(obj_rel)) catch @panic("OOM");

        // `.s` (assembly) goes through the C driver (`cc`/clang), never `c++`,
        // and skips the C/C++ compile flags -- but still honors include dirs
        // (a .S may #include <picolibc.h> etc.).
        const is_s = ext.len != 0 and std.ascii.eqlIgnoreCase(ext, ".s");
        const use_cpp = if (is_s) false else opts.is_cpp;

        const compile = b.addSystemCommand(&.{tc.compilerExe(b, use_cpp)});
        // zig needs the `cc`/`c++` subcommand; a bare clang/clang++ exe does not.
        if (tc.isZig()) compile.addArg(if (use_cpp) "c++" else "cc");
        // Xbox CPU is a Pentium III (Coppermine): MMX + SSE1, no SSE2. Pin the
        // target CPU so clang never emits SSE2 (e.g. for 64-bit integer math),
        // which faults as STATUS_ILLEGAL_INSTRUCTION on hardware.
        compile.addArg("-march=pentium3");
        // zig spells the triple `-target x86-windows-*`; clang wants
        // `--target=i686-pc-windows-*` (mapped by toolchain.targetTriple).
        if (tc.isZig()) {
            compile.addArgs(&.{ "-target", target });
        } else {
            compile.addArg(b.fmt("--target={s}", .{target}));
        }
        compile.addArgs(&.{ "-c", "-o" });
        compile.addArg(obj_rel);
        if (!is_s) compile.addArgs(opts.flags);
        compile.addArg(opts.opt_flag);
        // llvm mode: this clang's -nostdinc also strips its resource dir, so the
        // builtin headers (stddef.h/stdarg.h/…) must be re-added. Searched after
        // the -I dirs below (system category), so RXDK's own headers still win.
        if (!tc.isZig() and tc.builtin_include.len != 0) {
            compile.addArgs(&.{ "-isystem", tc.builtin_include });
        }
        for (opts.include_dirs) |inc| {
            compile.addArg(b.fmt("-I{s}", .{inc}));
        }
        compile.addFileArg(b.path(src));
        compile.setCwd(b.path("."));
        compile.step.dependOn(mkdir);
        steps.append(allocator, &compile.step) catch @panic("OOM");
    }

    const aggregate = b.allocator.create(std.Build.Step) catch @panic("OOM");
    aggregate.* = std.Build.Step.init(.{
        .id = .custom,
        .name = b.fmt("compile-c-{s}", .{opts.name}),
        .owner = b,
    });
    for (steps.items) |dep| {
        aggregate.dependOn(dep);
    }

    return .{
        .step = aggregate,
        .outputs = outputs.toOwnedSlice(allocator) catch @panic("OOM"),
    };
}
