// libd3dx8 source manifest — the Xbox D3DX8 helper library ported from the
// May-2020 leak (private/windows/directx/dxg/d3dx8). Mirrors libd3d8/sources.zig.
//
// d3dx8.lib is a composite of 10 sub-libraries (see d3dx8/link/sources):
//   D3DX proper:  core, math, tex, mesh, shape, effect, xof6 (.X file parser)
//   3rd-party:    jpeglib, lpng105 (libpng), zlib113   (image-file codecs for tex)
//
// Unlike libd3d8 (the kernel-runtime NV2A driver), this is TITLE-SIDE user-mode
// code: every TU pulls a per-component pch*.h that sets up xtl.h + the public
// d3d8.h + standard C headers (see site/bridge_d3dx8.h). Each slice lists exactly
// the SOURCES= set the original component's `sources` makefile compiled (the dirs
// carry extra files — load3ds/loadmesh/savemesh, CD3DXEffect*, png test files —
// that the leak build did not link).

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
};

// --- core: D3DXCreate*, sprite, buffer, .X file, render-to-surface/envmap ---
pub const core_cpp_sources = [_][]const u8{
    "libs/libd3dx8/core/d3dx8core.cpp",
    "libs/libd3dx8/core/d3dx8dbg.cpp",
    "libs/libd3dx8/core/CD3DXSprite.cpp",
    "libs/libd3dx8/core/CD3DXBuffer.cpp",
    "libs/libd3dx8/core/CD3DXFile.cpp",
    "libs/libd3dx8/core/CD3DXRenderToSurface.cpp",
    "libs/libd3dx8/core/cd3dxrendertoenvmap.cpp",
    "libs/libd3dx8/core/CD3DXStack.cpp",
};
pub const core_c_sources = [_][]const u8{
    "libs/libd3dx8/core/init.c",
};

// --- math: matrices, quaternions, vectors, splines, matrix stack ---
pub const math_sources = [_][]const u8{
    "libs/libd3dx8/math/d3dxmath.cpp",
    "libs/libd3dx8/math/cstack.cpp",
};

// --- tex: texture load/save, image codecs, S3TC compress, blt ---
pub const tex_sources = [_][]const u8{
    "libs/libd3dx8/tex/d3dx8tex.cpp",
    "libs/libd3dx8/tex/CD3DXImage.cpp",
    "libs/libd3dx8/tex/CD3DXCodec.cpp",
    "libs/libd3dx8/tex/CD3DXBlt.cpp",
    "libs/libd3dx8/tex/s3tc.cpp",
    "libs/libd3dx8/tex/s3tchelp.cpp",
};

// --- mesh: create/clean/skin/optimize, intersect, .X loading ---
pub const mesh_sources = [_][]const u8{
    "libs/libd3dx8/mesh/quadric.cpp",
    "libs/libd3dx8/mesh/createmesh.cpp",
    "libs/libd3dx8/mesh/loadutil.cpp",
    "libs/libd3dx8/mesh/btri.cpp",
    "libs/libd3dx8/mesh/intersect.cpp",
    "libs/libd3dx8/mesh/cleanmesh.cpp",
    "libs/libd3dx8/mesh/loadx.cpp",
    "libs/libd3dx8/mesh/skinmesh.cpp",
};

// --- shape: box/sphere/cylinder/teapot/text geometry (shapes.cpp #includes teapot.cpp) ---
pub const shape_sources = [_][]const u8{
    "libs/libd3dx8/shape/shapes.cpp",
};

// --- effect: .fx compiler, technique/declaration/parser ---
pub const effect_sources = [_][]const u8{
    "libs/libd3dx8/effect/ccompiler.cpp",
    "libs/libd3dx8/effect/cdeclaration.cpp",
    "libs/libd3dx8/effect/ceffect.cpp",
    "libs/libd3dx8/effect/ctechnique.cpp",
    "libs/libd3dx8/effect/d3dx8effect.cpp",
};

// --- xof6 (d3dxof): the DirectX .X file format parser ---
pub const xof_cpp_sources = [_][]const u8{
    "libs/libd3dx8/xof6/xblob.cpp",
    "libs/libd3dx8/xof6/xdata.cpp",
    "libs/libd3dx8/xof6/xfactory.cpp",
    "libs/libd3dx8/xof6/ximplapi.cpp",
    "libs/libd3dx8/xof6/xmemory.cpp",
    "libs/libd3dx8/xof6/xobject.cpp",
    "libs/libd3dx8/xof6/xparse.cpp",
    "libs/libd3dx8/xof6/xprim.cpp",
    "libs/libd3dx8/xof6/xstring.cpp",
    "libs/libd3dx8/xof6/xstrmrd.cpp",
    "libs/libd3dx8/xof6/xtempl.cpp",
    "libs/libd3dx8/xof6/xzip.cpp",
};
pub const xof_c_sources = [_][]const u8{
    "libs/libd3dx8/xof6/nfmcomp.c",
    "libs/libd3dx8/xof6/nfmdeco.c",
    "libs/libd3dx8/xof6/xguid.c",
};

// --- 3rd-party: jpeglib (libjpeg, .c renamed to .cpp in the leak) ---
pub const jpeglib_sources = [_][]const u8{
    "libs/libd3dx8/misc/jpeglib/jcomapi.cpp",
    "libs/libd3dx8/misc/jpeglib/jdapimin.cpp",
    "libs/libd3dx8/misc/jpeglib/jdapistd.cpp",
    "libs/libd3dx8/misc/jpeglib/jdcoefct.cpp",
    "libs/libd3dx8/misc/jpeglib/jdcolor.cpp",
    "libs/libd3dx8/misc/jpeglib/jddctmgr.cpp",
    "libs/libd3dx8/misc/jpeglib/jdhuff.cpp",
    "libs/libd3dx8/misc/jpeglib/jdinput.cpp",
    "libs/libd3dx8/misc/jpeglib/jdmainct.cpp",
    "libs/libd3dx8/misc/jpeglib/jdmarker.cpp",
    "libs/libd3dx8/misc/jpeglib/jdmaster.cpp",
    "libs/libd3dx8/misc/jpeglib/jdmerge.cpp",
    "libs/libd3dx8/misc/jpeglib/jdphuff.cpp",
    "libs/libd3dx8/misc/jpeglib/jdpostct.cpp",
    "libs/libd3dx8/misc/jpeglib/jdsample.cpp",
    "libs/libd3dx8/misc/jpeglib/jdtrans.cpp",
    "libs/libd3dx8/misc/jpeglib/jerror.cpp",
    "libs/libd3dx8/misc/jpeglib/jidctflt.cpp",
    "libs/libd3dx8/misc/jpeglib/jidctfst.cpp",
    "libs/libd3dx8/misc/jpeglib/jidctint.cpp",
    "libs/libd3dx8/misc/jpeglib/jmemmgr.cpp",
    "libs/libd3dx8/misc/jpeglib/jmemnobs.cpp",
    "libs/libd3dx8/misc/jpeglib/jquant1.cpp",
    "libs/libd3dx8/misc/jpeglib/jquant2.cpp",
    "libs/libd3dx8/misc/jpeglib/jutils.cpp",
    "libs/libd3dx8/misc/jpeglib/miint.cpp",
    "libs/libd3dx8/misc/jpeglib/mifst.cpp",
    "libs/libd3dx8/misc/jpeglib/mfint.cpp",
    "libs/libd3dx8/misc/jpeglib/mffst.cpp",
    "libs/libd3dx8/misc/jpeglib/piint.cpp",
    "libs/libd3dx8/misc/jpeglib/pifst.cpp",
    "libs/libd3dx8/misc/jpeglib/pfint.cpp",
    "libs/libd3dx8/misc/jpeglib/pffst.cpp",
};

// --- 3rd-party: lpng105 (libpng 1.0.5) ---
pub const lpng_sources = [_][]const u8{
    "libs/libd3dx8/misc/lpng105/png.cpp",
    "libs/libd3dx8/misc/lpng105/pngset.cpp",
    "libs/libd3dx8/misc/lpng105/pngget.cpp",
    "libs/libd3dx8/misc/lpng105/pngread.cpp",
    "libs/libd3dx8/misc/lpng105/pngpread.cpp",
    "libs/libd3dx8/misc/lpng105/pngrtran.cpp",
    "libs/libd3dx8/misc/lpng105/pngrutil.cpp",
    "libs/libd3dx8/misc/lpng105/pngerror.cpp",
    "libs/libd3dx8/misc/lpng105/pngmem.cpp",
    "libs/libd3dx8/misc/lpng105/pngrio.cpp",
    "libs/libd3dx8/misc/lpng105/pngwio.cpp",
    "libs/libd3dx8/misc/lpng105/pngtrans.cpp",
    "libs/libd3dx8/misc/lpng105/pngwrite.cpp",
    "libs/libd3dx8/misc/lpng105/pngwtran.cpp",
    "libs/libd3dx8/misc/lpng105/pngwutil.cpp",
};

// --- 3rd-party: zlib113 (zlib 1.1.3) ---
pub const zlib_sources = [_][]const u8{
    "libs/libd3dx8/misc/zlib113/adler32.cpp",
    "libs/libd3dx8/misc/zlib113/compress.cpp",
    "libs/libd3dx8/misc/zlib113/crc32.cpp",
    "libs/libd3dx8/misc/zlib113/deflate.cpp",
    "libs/libd3dx8/misc/zlib113/gzio.cpp",
    "libs/libd3dx8/misc/zlib113/infblock.cpp",
    "libs/libd3dx8/misc/zlib113/infcodes.cpp",
    "libs/libd3dx8/misc/zlib113/inffast.cpp",
    "libs/libd3dx8/misc/zlib113/inflate.cpp",
    "libs/libd3dx8/misc/zlib113/inftrees.cpp",
    "libs/libd3dx8/misc/zlib113/infutil.cpp",
    "libs/libd3dx8/misc/zlib113/maketree.cpp",
    "libs/libd3dx8/misc/zlib113/trees.cpp",
    "libs/libd3dx8/misc/zlib113/uncompr.cpp",
    "libs/libd3dx8/misc/zlib113/zutil.cpp",
};

pub const slices = [_]Slice{
    .{ .name = "core", .is_cpp = true, .sources = &core_cpp_sources },
    .{ .name = "core-c", .is_cpp = false, .sources = &core_c_sources },
    .{ .name = "math", .is_cpp = true, .sources = &math_sources },
    .{ .name = "tex", .is_cpp = true, .sources = &tex_sources },
    .{ .name = "mesh", .is_cpp = true, .sources = &mesh_sources },
    .{ .name = "shape", .is_cpp = true, .sources = &shape_sources },
    .{ .name = "effect", .is_cpp = true, .sources = &effect_sources },
    .{ .name = "xof", .is_cpp = true, .sources = &xof_cpp_sources },
    .{ .name = "xof-c", .is_cpp = false, .sources = &xof_c_sources },
    .{ .name = "jpeglib", .is_cpp = true, .sources = &jpeglib_sources },
    .{ .name = "lpng105", .is_cpp = true, .sources = &lpng_sources },
    .{ .name = "zlib113", .is_cpp = true, .sources = &zlib_sources },
};
