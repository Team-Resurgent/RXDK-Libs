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
        // See picolibcFlags()'s -fno-builtin comment: without this, Clang is free
        // to recognize a memmove/memcpy/RtlMoveMemory-shaped call site as a known
        // builtin and inline-expand it directly at the call site (bypassing the
        // fixed picolibc implementation entirely), rather than emit a genuine
        // external call. Applies everywhere a call site could be miscompiled,
        // not just inside picolibc's own implementation.
        "-fno-builtin",
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
        "-fno-builtin",
        // C++ thread_local storage: emulated TLS (a per-thread table reached via
        // __emutls_get_address) instead of the Windows __tls_index/TEB model, which
        // raw libc/libcpp threads don't set up. Backed by libc tss (see emutls.c).
        "-femulated-tls",
        // ...but clang still emits a CodeView S_*THREAD32 debug record for each
        // thread_local pointing at the native symbol (_g), which -femulated-tls never
        // defines -> undefined-symbol at link. Debug info isn't consumed in the XBE
        // pipeline, so drop it (-g0) to suppress the dangling TLS debug records.
        "-g0",
    };
}

pub fn picolibcFlags(_: *std.Build) []const []const u8 {
    return &.{
        "-std=c17",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fno-zero-initialized-in-bss",
        "-fno-sanitize=undefined",
        // picolibc's memcpy/memmove/memset/strlen etc. (libc/string) rely on
        // their `__no_builtin` function attribute to stop the compiler from
        // recognizing their own manual copy/scan loops as a memcpy/memmove/
        // strlen idiom and substituting a call back to that same builtin --
        // which, since these functions ARE that builtin's real implementation,
        // is direct infinite self-recursion (no stack growth if tail-call
        // folded, so it presents as a silent hang, not a crash or stack
        // overflow fault). Confirmed on real hardware/xemu: RtlCreateHeap's
        // 48-byte RtlMoveMemory(&TempParameters, Parameters, ...) call (see
        // libs/libxapi/rtl/heap.c) hung indefinitely and was traced to this.
        // `__no_builtin` alone isn't reliably honored by Clang on this niche
        // freestanding x86-windows-gnu target; `-fno-builtin` is the strong,
        // translation-unit-wide guarantee and is the right fix for a libc
        // implementation regardless.
        "-fno-builtin",
        // Vendored picolibc — silence its upstream warnings (macro redefines
        // from our force-included picolibc.h, wint_t va_arg, etc.), matching
        // how the first-party cFlags / libxapi / libcxx batches are compiled.
        "-Wno-everything",
        "-D__Picolibc__",
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
        "-fno-builtin",
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
        "-fno-builtin",
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
    mod.addIncludePath(b.path("build/generated"));
    mod.addIncludePath(b.path("shared/picolibc/include"));
    mod.addIncludePath(b.path("shared/picolibc/machine/x86"));
}

pub fn addLibcxxIncludes(mod: *std.Build.Module, b: *std.Build) void {
    addIncludeTree(mod, b);
    mod.addIncludePath(b.path("shared/libcxx/include"));
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
