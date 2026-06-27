const std = @import("std");
const compile_c = @import("../../build/compile_c.zig");
const xbox_target = @import("../../build/xbox_target.zig");

// LLVM libunwind, DWARF engine, built baremetal: frame discovery uses the
// linker-provided __eh_frame_start/__eh_frame_end symbols (see AddressSpace.hpp
// findUnwindSections, _LIBUNWIND_IS_BAREMETAL branch) rather than the Windows
// EnumProcessModules path, which the bare XBE has no loader for. _WIN32 stays
// defined so arch detection + the DWARF (no-SEH) model are selected.

const include_dirs = [_][]const u8{
    // Shadows mingw <windows.h>/<ntverp.h> (pulled under _WIN32) with empty
    // stubs -- we use the DWARF unwinder, not SEH, so no Win32 API is needed,
    // and the real windows.h drags in stralign.h -> wcscpy. Must be first.
    "build/generated/libunwind_stubs",
    "vendor/llvm-project/libunwind/include",
    "vendor/llvm-project/libunwind/src",
    "build/generated",
    "shared/picolibc/include",
    "shared/picolibc/machine/x86",
};

const extra = [_][]const u8{
    "-D_LIBUNWIND_IS_BAREMETAL",
    // No SRWLOCK on Xbox; we use static .eh_frame discovery (not the dynamic-FDE
    // cache), so libunwind's RWMutex can be a no-op.
    "-D_LIBUNWIND_HAS_NO_THREADS",
    "-funwind-tables",
    "-Wno-everything",
    "-include",
    "picolibc.h",
};

const cpp_sources = [_][]const u8{
    "vendor/llvm-project/libunwind/src/libunwind.cpp",
};

const c_sources = [_][]const u8{
    "vendor/llvm-project/libunwind/src/UnwindLevel1.c",
    "vendor/llvm-project/libunwind/src/UnwindLevel1-gcc-ext.c",
    "vendor/llvm-project/libunwind/src/UnwindRegistersSave.S",
    "vendor/llvm-project/libunwind/src/UnwindRegistersRestore.S",
};

pub fn addUnwindObjects(b: *std.Build, opt_flag: []const u8) compile_c.CompileBatch {
    const cpp = compile_c.addBatch(b, .{
        .name = "libunwind",
        .target = xbox_target.target_triple,
        .out_subdir = "libunwind",
        .sources = &cpp_sources,
        .flags = xbox_target.appendFlags(b, xbox_target.cppFlags(b), &extra),
        .include_dirs = &include_dirs,
        .opt_flag = opt_flag,
        .is_cpp = true,
    });
    const c = compile_c.addBatch(b, .{
        .name = "libunwind-c",
        .target = xbox_target.target_triple,
        .out_subdir = "libunwind",
        .sources = &c_sources,
        .flags = xbox_target.appendFlags(b, xbox_target.cFlags(b), &extra),
        .include_dirs = &include_dirs,
        .opt_flag = opt_flag,
        .is_cpp = false,
    });

    var outputs = std.ArrayListUnmanaged(std.Build.LazyPath).empty;
    outputs.appendSlice(b.allocator, cpp.outputs) catch @panic("OOM");
    outputs.appendSlice(b.allocator, c.outputs) catch @panic("OOM");

    const step = b.allocator.create(std.Build.Step) catch @panic("OOM");
    step.* = std.Build.Step.init(.{ .id = .custom, .name = "libunwind", .owner = b });
    step.dependOn(cpp.step);
    step.dependOn(c.step);

    return .{ .step = step, .outputs = outputs.toOwnedSlice(b.allocator) catch @panic("OOM") };
}
