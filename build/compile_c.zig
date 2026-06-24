const std = @import("std");

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

        const compile = b.addSystemCommand(&.{b.graph.zig_exe});
        if (ext.len != 0 and std.ascii.eqlIgnoreCase(ext, ".s")) {
            compile.addArg("cc");
            compile.addArgs(&.{ "-target", opts.target, "-c", "-o" });
            compile.addArg(obj_rel);
            compile.addArg(opts.opt_flag);
            compile.addFileArg(b.path(src));
        } else {
            compile.addArg(if (opts.is_cpp) "c++" else "cc");
            compile.addArgs(&.{ "-target", opts.target, "-c", "-o" });
            compile.addArg(obj_rel);
            compile.addArgs(opts.flags);
            compile.addArg(opts.opt_flag);
            for (opts.include_dirs) |inc| {
                compile.addArg(b.fmt("-I{s}", .{inc}));
            }
            compile.addFileArg(b.path(src));
        }
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
