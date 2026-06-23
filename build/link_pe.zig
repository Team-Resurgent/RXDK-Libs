const std = @import("std");

const RspInfo = struct {
    step: *std.Build.Step,
    rsp_path: []const u8,
};

const WriteRspContext = struct {
    step: std.Build.Step,
    b: *std.Build,
    name: []const u8,
    object_paths: []const std.Build.LazyPath,
    rsp_path: []const u8,
};

pub fn addObjectRsp(
    b: *std.Build,
    name: []const u8,
    object_paths: []const std.Build.LazyPath,
) RspInfo {
    const rsp_path = b.fmt("zig-out/link/{s}.rsp", .{name});
    const ctx = b.allocator.create(WriteRspContext) catch @panic("OOM");
    ctx.* = .{
        .step = std.Build.Step.init(.{
            .id = .custom,
            .name = b.fmt("link-rsp-{s}", .{name}),
            .owner = b,
        }),
        .b = b,
        .name = name,
        .object_paths = object_paths,
        .rsp_path = rsp_path,
    };
    ctx.step.makeFn = writeRsp;
    return .{ .step = &ctx.step, .rsp_path = rsp_path };
}

fn writeRsp(step: *std.Build.Step, options: std.Build.Step.MakeOptions) !void {
    _ = options;
    const ctx: *WriteRspContext = @fieldParentPtr("step", step);
    const b = ctx.b;
    const io = b.graph.io;
    const allocator = b.allocator;

    try b.build_root.handle.createDirPath(io, "zig-out/link");
    var rsp_body = std.ArrayListUnmanaged(u8).empty;
    defer rsp_body.deinit(allocator);
    for (ctx.object_paths) |obj| {
        const path = obj.getPath(b);
        try rsp_body.appendSlice(allocator, "\"");
        try rsp_body.appendSlice(allocator, path);
        try rsp_body.appendSlice(allocator, "\"\r\n");
    }
    try b.build_root.handle.writeFile(io, .{ .sub_path = ctx.rsp_path, .data = rsp_body.items });
}

pub const Options = struct {
    name: []const u8,
    src: []const u8,
    is_cpp: bool = false,
    libs: []const std.Build.LazyPath = &.{},
    objects: []const std.Build.LazyPath = &.{},
    include_paths: []const std.Build.LazyPath = &.{},
    extra_flags: []const []const u8 = &.{},
    entry: []const u8 = "start",
    deps: []const *std.Build.Step = &.{},
    /// Probe-link PE, emit zig-out/link/<name>_image_init.h, then link image_init.o.
    bootstrap: bool = false,
};

const MkdirContext = struct {
    step: std.Build.Step,
    b: *std.Build,
    out_dir: []const u8,
};

fn addMkdir(b: *std.Build, out_dir: []const u8) *std.Build.Step {
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
    ctx.step.makeFn = makeMkdir;
    return &ctx.step;
}

fn makeMkdir(step: *std.Build.Step, options: std.Build.Step.MakeOptions) !void {
    _ = options;
    const ctx: *MkdirContext = @fieldParentPtr("step", step);
    try ctx.b.build_root.handle.createDirPath(ctx.b.graph.io, ctx.out_dir);
}

fn addLinkExe(
    b: *std.Build,
    xbox_target: @TypeOf(@import("xbox_target.zig")),
    opt_flag: []const u8,
    link_flags: []const []const u8,
    out_exe: []const u8,
    sample_obj: []const u8,
    libs: []const std.Build.LazyPath,
    rsp: ?RspInfo,
    extra_objs: []const []const u8,
    entry: []const u8,
    deps: []const *std.Build.Step,
) *std.Build.Step {
    const link = b.addSystemCommand(&.{
        b.graph.zig_exe, "cc", "-target", xbox_target.target_triple,
    });
    link.addArgs(&.{ "-o" });
    link.addArg(out_exe);
    link.addArg(sample_obj);
    if (libs.len > 0) {
        link.addFileArg(b.path("prebuilt/xboxkrnl_xbld.obj"));
    }
    for (libs) |lib| {
        link.addFileArg(lib);
    }
    if (rsp) |r| {
        link.addArg(b.fmt("@{s}", .{r.rsp_path}));
        link.step.dependOn(r.step);
    }
    for (extra_objs) |obj| {
        link.addArg(obj);
    }
    link.addArgs(link_flags);
    link.addArg(opt_flag);
    link.addArg("-rtlib=compiler-rt");
    link.addArg("-e");
    link.addArg(entry);
    link.setCwd(b.path("."));
    for (deps) |dep| {
        link.step.dependOn(dep);
    }
    return &link.step;
}

pub fn addPeSample(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    xbox_target: @TypeOf(@import("xbox_target.zig")),
    opts: Options,
) struct { step: *std.Build.Step, install: *std.Build.Step } {
    _ = target;
    const opt_flag: []const u8 = switch (optimize) {
        .Debug => "-O0",
        .ReleaseSafe => "-O2",
        .ReleaseFast => "-O3",
        .ReleaseSmall => "-Os",
    };
    const out_dir = b.fmt("zig-out/samples/{s}", .{opts.name});
    const obj_path = b.fmt("{s}/{s}.o", .{ out_dir, opts.name });
    const exe_path = b.fmt("{s}/{s}.exe", .{ out_dir, opts.name });
    const link_dir_mkdir = addMkdir(b, "zig-out/link");
    const mkdir = addMkdir(b, out_dir);

    const std_flags = if (opts.is_cpp) xbox_target.cppFlags(b) else xbox_target.cFlags(b);
    const link_flags = xbox_target.peLinkFlags(b);

    const compile = b.addSystemCommand(&.{
        b.graph.zig_exe, "cc", "-target", xbox_target.target_triple,
    });
    compile.addArgs(&.{ "-c", "-o" });
    compile.addArg(obj_path);
    compile.addArgs(std_flags);
    compile.addArg(opt_flag);
    compile.addArgs(opts.extra_flags);
    if (!opts.is_cpp) {
        compile.addArg("-D_XBOX=1");
        compile.addArg("-D_WIN32=1");
    } else {
        compile.addArg("-D_XBOX=1");
    }
    for (opts.include_paths) |inc| {
        compile.addArg(b.fmt("-I{s}", .{inc.getPath(b)}));
    }
    compile.addFileArg(b.path(opts.src));
    compile.setCwd(b.path("."));
    compile.step.dependOn(mkdir);

    const object_rsp: ?RspInfo = if (opts.objects.len > 0)
        addObjectRsp(b, opts.name, opts.objects)
    else
        null;

    var link_deps = std.ArrayListUnmanaged(*std.Build.Step).empty;
    link_deps.append(b.allocator, &compile.step) catch @panic("OOM");
    link_deps.append(b.allocator, mkdir) catch @panic("OOM");
    for (opts.deps) |dep| {
        compile.step.dependOn(dep);
        link_deps.append(b.allocator, dep) catch @panic("OOM");
    }

    const final_link = if (opts.bootstrap) blk: {
        const probe_exe = b.fmt("zig-out/link/{s}_probe.exe", .{opts.name});
        const probe_image_init_obj = b.fmt("zig-out/link/{s}_image_init_probe.o", .{opts.name});
        const image_init_obj = b.fmt("zig-out/link/{s}_image_init.o", .{opts.name});

        const compile_probe_image_init = b.addSystemCommand(&.{
            b.graph.zig_exe, "cc", "-target", xbox_target.target_triple,
        });
        compile_probe_image_init.addArgs(&.{ "-c", "-o" });
        compile_probe_image_init.addArg(probe_image_init_obj);
        compile_probe_image_init.addArgs(xbox_target.cFlags(b));
        compile_probe_image_init.addArg(opt_flag);
        compile_probe_image_init.addArg("-D_XBOX=1");
        compile_probe_image_init.addArg("-D_WIN32=1");
        compile_probe_image_init.addArg("-Iinclude");
        compile_probe_image_init.addArg("-Ibuild/generated");
        compile_probe_image_init.addArg("-include");
        compile_probe_image_init.addArg("build/generated/xbox_image_init_stub.h");
        compile_probe_image_init.addFileArg(b.path("src/xbox/image_init.c"));
        compile_probe_image_init.setCwd(b.path("."));
        compile_probe_image_init.step.dependOn(link_dir_mkdir);

        const probe_link = addLinkExe(
            b, xbox_target, opt_flag, link_flags, probe_exe, obj_path,
            opts.libs, object_rsp, &.{probe_image_init_obj}, opts.entry, link_deps.items,
        );
        probe_link.dependOn(&compile_probe_image_init.step);
        probe_link.dependOn(link_dir_mkdir);

        const patch_probe = b.addSystemCommand(&.{
            "powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
            "-File", "scripts/Patch-PeXbox.ps1",
            "-Path",
        });
        patch_probe.addArg(probe_exe);
        patch_probe.setCwd(b.path("."));
        patch_probe.step.dependOn(probe_link);

        const gen_header = b.addSystemCommand(&.{
            "powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
            "-File", "scripts/Write-XboxImageInit.ps1",
            "-Name", opts.name,
            "-InputExe",
        });
        gen_header.addArg(probe_exe);
        gen_header.setCwd(b.path("."));
        gen_header.step.dependOn(&patch_probe.step);

        const compile_image_init = b.addSystemCommand(&.{
            b.graph.zig_exe, "cc", "-target", xbox_target.target_triple,
        });
        compile_image_init.addArgs(&.{ "-c", "-o" });
        compile_image_init.addArg(image_init_obj);
        compile_image_init.addArgs(xbox_target.cFlags(b));
        compile_image_init.addArg(opt_flag);
        compile_image_init.addArg("-D_XBOX=1");
        compile_image_init.addArg("-D_WIN32=1");
        compile_image_init.addArg("-Iinclude");
        compile_image_init.addArg("-Ibuild/generated");
        compile_image_init.addArg("-Izig-out/link");
        compile_image_init.addArg("-include");
        compile_image_init.addArg(b.fmt("zig-out/link/{s}_image_init.h", .{opts.name}));
        compile_image_init.addFileArg(b.path("src/xbox/image_init.c"));
        compile_image_init.setCwd(b.path("."));
        compile_image_init.step.dependOn(&gen_header.step);
        compile_image_init.step.dependOn(link_dir_mkdir);

        const link = addLinkExe(
            b, xbox_target, opt_flag, link_flags, exe_path, obj_path,
            opts.libs, object_rsp, &.{image_init_obj}, opts.entry, link_deps.items,
        );
        link.dependOn(&compile_image_init.step);
        link.dependOn(link_dir_mkdir);
        break :blk link;
    } else blk: {
        const link = addLinkExe(
            b, xbox_target, opt_flag, link_flags, exe_path, obj_path,
            opts.libs, object_rsp, &.{}, opts.entry, link_deps.items,
        );
        break :blk link;
    };

    const patch = b.addSystemCommand(&.{
        "powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
        "-File", "scripts/Patch-PeXbox.ps1",
        "-Path",
    });
    patch.addArg(exe_path);
    patch.setCwd(b.path("."));
    patch.step.dependOn(final_link);

    const install = b.addInstallFile(b.path(exe_path), b.fmt("samples/{s}/{s}.exe", .{ opts.name, opts.name }));
    install.step.dependOn(&patch.step);

    return .{ .step = &patch.step, .install = &install.step };
}
