const std = @import("std");

const forbidden = [_][]const u8{
    "cl.exe",
    "link.exe",
    "lib.exe",
    "msbuild",
    "Find-VsTool",
    "vcvars",
};

pub fn addStep(b: *std.Build) *std.Build.Step {
    const step = b.allocator.create(std.Build.Step) catch @panic("OOM");
    step.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "verify-no-vs",
        .owner = b,
    });
    step.makeFn = make;
    return step;
}

fn make(step: *std.Build.Step, options: std.Build.Step.MakeOptions) !void {
    _ = options;
    const b = step.owner;
    const io = b.graph.io;
    const allocator = b.allocator;

    var dir = try b.build_root.handle.openDir(io, "build", .{ .iterate = true });
    defer dir.close(io);

    var it = dir.iterate();
    while (try it.next(io)) |entry| {
        if (entry.kind != .file) continue;
        if (!std.mem.endsWith(u8, entry.name, ".zig")) continue;
        if (std.mem.eql(u8, entry.name, "verify_no_vs.zig")) continue;
        const rel = try std.fmt.allocPrint(allocator, "build/{s}", .{entry.name});
        defer allocator.free(rel);
        const data = try b.build_root.handle.readFileAlloc(io, rel, allocator, .unlimited);
        defer allocator.free(data);
        for (forbidden) |needle| {
            if (std.mem.indexOf(u8, data, needle) != null) {
                std.log.err("forbidden token '{s}' in {s}", .{ needle, rel });
                return error.ForbiddenToolchainToken;
            }
        }
    }
}
