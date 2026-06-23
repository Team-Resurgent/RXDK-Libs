const std = @import("std");

pub fn main() !void {
    const allocator = std.heap.page_allocator;

    const in_path = "prebuilt/xboxkrnl.lib";
    const out_dir = "prebuilt/xboxkrnl_imports";

    const data = try std.fs.cwd().readFileAlloc(allocator, in_path, 64 * 1024 * 1024);
    defer allocator.free(data);

    try std.fs.cwd().makePath(out_dir);

    var index: usize = 0;
    if (!std.mem.startsWith(u8, data, "!<arch>\n")) {
        std.log.err("not a unix ar archive: {s}", .{in_path});
        return error.BadArchive;
    }
    index += 8;

    var n: u32 = 0;
    while (index + 60 <= data.len) {
        const header = data[index .. index + 60];
        index += 60;
        const size_field = std.mem.trimRight(u8, header[48..58], " ");
        const size = try std.fmt.parseInt(usize, size_field, 10);
        if (index + size > data.len) break;
        const member = data[index .. index + size];
        index += size;
        if (index & 1 == 1) index += 1;

        const out_name = try std.fmt.allocPrint(allocator, "{s}/import_{d}.obj", .{ out_dir, n });
        defer allocator.free(out_name);
        try std.fs.cwd().writeFile(.{ .sub_path = out_name, .data = member });
        n += 1;
    }

    std.log.info("extracted {d} import objects to {s}", .{ n, out_dir });
}
