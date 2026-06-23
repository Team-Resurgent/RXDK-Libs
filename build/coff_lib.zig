const std = @import("std");

pub const PackResult = struct {
    path: std.Build.LazyPath,
    step: *std.Build.Step,
};

const WriteRspContext = struct {
    step: std.Build.Step,
    b: *std.Build,
    name: []const u8,
    object_paths: []const std.Build.LazyPath,
};

pub fn pack(
    b: *std.Build,
    name: []const u8,
    object_paths: []const std.Build.LazyPath,
    deps: []const *std.Build.Step,
) PackResult {
    const allocator = b.allocator;
    const lib_path = b.fmt("zig-out/lib/{s}.lib", .{name});
    const rsp_path = b.fmt("zig-out/lib/{s}.rsp", .{name});

    const write_ctx = allocator.create(WriteRspContext) catch @panic("OOM");
    write_ctx.* = .{
        .step = std.Build.Step.init(.{
            .id = .custom,
            .name = b.fmt("write-rsp-{s}", .{name}),
            .owner = b,
        }),
        .b = b,
        .name = name,
        .object_paths = object_paths,
    };
    write_ctx.step.makeFn = writeRsp;

    const run = b.addSystemCommand(&.{ b.graph.zig_exe, "lib", "/NOLOGO" });
    run.addArg(b.fmt("/OUT:{s}", .{lib_path}));
    run.addArg(b.fmt("@{s}", .{rsp_path}));
    run.setCwd(b.path("."));
    run.step.dependOn(&write_ctx.step);
    for (deps) |dep| {
        run.step.dependOn(dep);
    }

    return .{
        .path = b.path(lib_path),
        .step = &run.step,
    };
}

fn writeRsp(step: *std.Build.Step, options: std.Build.Step.MakeOptions) !void {
    _ = options;
    const ctx: *WriteRspContext = @fieldParentPtr("step", step);
    const b = ctx.b;
    const io = b.graph.io;
    const allocator = b.allocator;

    try b.build_root.handle.createDirPath(io, "zig-out/lib");
    const rsp_path = b.fmt("zig-out/lib/{s}.rsp", .{ctx.name});

    var rsp_body = std.ArrayListUnmanaged(u8).empty;
    defer rsp_body.deinit(allocator);
    for (ctx.object_paths) |obj| {
        const path = obj.getPath(b);
        try rsp_body.appendSlice(allocator, "\"");
        try rsp_body.appendSlice(allocator, path);
        try rsp_body.appendSlice(allocator, "\"\r\n");
    }
    try b.build_root.handle.writeFile(io, .{ .sub_path = rsp_path, .data = rsp_body.items });
}
