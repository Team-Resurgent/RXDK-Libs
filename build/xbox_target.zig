const std = @import("std");

pub const target_query: std.Target.Query = .{
    .cpu_arch = .x86,
    .os_tag = .windows,
    .abi = .gnu,
};

pub const target_triple = "x86-windows-gnu";

pub fn resolveTarget(b: *std.Build) std.Build.ResolvedTarget {
    return b.resolveTargetQuery(target_query);
}

pub fn cFlags(_: *std.Build) []const []const u8 {
    return &.{
        "-std=c23",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fno-zero-initialized-in-bss",
        "-fdata-sections",
        "-ffunction-sections",
        "-fno-sanitize=undefined",
    };
}

pub fn cppFlags(_: *std.Build) []const []const u8 {
    return &.{
        "-std=c++23",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fdata-sections",
        "-ffunction-sections",
        "-fno-exceptions",
        "-frtti",
        "-nostdinc++",
        "-fno-sanitize=undefined",
    };
}

pub fn picolibcFlags(_: *std.Build) []const []const u8 {
    return &.{
        "-std=c17",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fno-zero-initialized-in-bss",
        "-fno-sanitize=undefined",
        "-D__Picolibc__",
        "-D__SINGLE_THREAD",
        "-D__TINY_STDIO",
        "-include",
        "picolibc.h",
    };
}

pub fn peLinkFlags(_: *std.Build) []const []const u8 {
    return &.{
        "-nostdlib",
        "-nostartfiles",
        // XBE loads at XBEIMAGE_STANDARD_BASE_ADDRESS (0x10000). Linking here keeps
        // code/data VAs aligned with imagebld + runtime XeImageHeader().
        "-Wl,--image-base=0x10000",
    };
}

pub fn xapiCFlags(_: *std.Build) []const []const u8 {
    return &.{
        "-std=c17",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fno-zero-initialized-in-bss",
        "-fdata-sections",
        "-ffunction-sections",
        "-fno-sanitize=undefined",
        "-Wno-everything",
        "-nostdinc",
        "-fms-extensions",
        "-fms-compatibility",
        // Xbox xAPI copies the PE TLS image into KeGetCurrentThread()->TlsData;
        // emutls would leave the template empty and break fixed slot offsets.
        "-fno-emulated-tls",
        "-include",
        "picolibc.h",
        "-include",
        "xapi_site.h",
        "-D_XAPI_",
    };
}

pub fn xapiCppFlags(_: *std.Build) []const []const u8 {
    return &.{
        "-std=c++17",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fdata-sections",
        "-ffunction-sections",
        "-fno-exceptions",
        "-frtti",
        "-nostdinc",
        "-nostdinc++",
        "-fno-sanitize=undefined",
        "-Wno-everything",
        "-fms-extensions",
        "-fms-compatibility",
        "-include",
        "picolibc.h",
        "-include",
        "xapi_site.h",
        "-D_XAPI_",
    };
}

pub fn addIncludeTree(mod: *std.Build.Module, b: *std.Build) void {
    mod.addIncludePath(b.path("include"));
    mod.addIncludePath(b.path("build/generated"));
    mod.addIncludePath(b.path("vendor/picolibc/libc/include"));
    mod.addIncludePath(b.path("vendor/picolibc/libc/machine/x86"));
}

pub fn addLibcxxIncludes(mod: *std.Build.Module, b: *std.Build) void {
    addIncludeTree(mod, b);
    mod.addIncludePath(b.path("vendor/llvm-project/libcxx/include"));
    mod.addIncludePath(b.path("vendor/llvm-project/libcxxabi/include"));
}

pub fn appendFlags(
    b: *std.Build,
    base: []const []const u8,
    extra: []const []const u8,
) []const []const u8 {
    var list = std.ArrayListUnmanaged([]const u8).empty;
    list.appendSlice(b.allocator, base) catch @panic("OOM");
    list.appendSlice(b.allocator, extra) catch @panic("OOM");
    return list.toOwnedSlice(b.allocator) catch @panic("OOM");
}
