// libdmusic source manifest -- the Xbox DirectMusic runtime (dmusic.lib) ported
// from the May-2020 leak (private/windows/directx/dmusic). This is the full
// interactive-music stack: the software synthesizer (dmsynth), the performance/
// segment/track engine (dmime), the loader (dmloader), and the authored-content
// components -- bands (dmband), styles (dmstyle), chord/composition (dmcompos),
// scripting (dmscript) -- plus the central GUID TU (dmguids).
//
// XBOX-ONLY: the Windows/Win9x variants (dmusic16/dmusic32/win9x), the host tools
// (dmtool/SegPlayer/dmusprod) and the tests are excluded. The per-component
// SOURCES lists below are the EXACT xbox `SOURCES=` sets from each component's
// leak `sources.inc` (NOT every staged .cpp -- the leak staged Windows-variant
// duplicates like each component's own guids.cpp, the DDK kernel-mode dmsynth
// files adapter/midi/miniport/mix*/umsynth/syslink, and stdafx.cpp/ATL that the
// xbox user-mode dmusic.lib never builds). In the leak these compile as separate
// OBJLIBs merged into dmusic.lib; here every TU lands in one libdmusic.lib.
//
// shared/*.cpp (dmstrm/oledll/tlist/trackhelp/smartref) are NOT compiled here:
// each component that needs them carries its own local copy in its SOURCES (e.g.
// dmime/dmstrm.cpp, dmcompos/tlist.cpp), matching the leak's per-OBJLIB layout.

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
    // Component precompiled header, force-included first (leak PRECOMPILED_INCLUDE).
    // null for components the leak built without a PCH (dmguids/dmstyle/dmsynth).
    pch: ?[]const u8 = null,
    // Per-component C_DEFINES beyond the shared dmusic set (see build.zig).
    extra_defines: []const []const u8 = &.{},
};

const D = "libs/libdmusic/";

// dmguids: the central INITGUID translation unit -- defines every DirectMusic
// class/interface/param GUID once for the whole lib.
const dmguids_cpp = [_][]const u8{
    D ++ "dmguids/xguids.cpp",
    // RXDK-added: the standard OLE/COM interface IIDs the leak pulled from
    // uuid.lib (which RXDK does not have). Compiled here under -DINITGUID.
    D ++ "dmguids/comguids_rxdk.cpp",
};

// dmsynth: the software synthesizer (user-mode, XMIX=DirectSound HW mixing path).
const dmsynth_cpp = [_][]const u8{
    D ++ "dmsynth/clist.cpp",
    D ++ "dmsynth/control.cpp",
    D ++ "dmsynth/csynth.cpp",
    D ++ "dmsynth/instr.cpp",
    D ++ "dmsynth/voice.cpp",
    D ++ "dmsynth/guids_dmsynth.cpp",
    D ++ "dmsynth/wave.cpp",
    D ++ "dmsynth/dls.cpp",
    D ++ "dmsynth/dmrand.cpp",
    // NOTE: dsoundsequencer.cpp (the XMIX DirectSound HW-mix sequencer) is
    // EXCLUDED: unlike the rest of dmusic it is compiled in the DirectSound
    // *driver* environment -- it pulls <nt.h>/<ntos.h> + libdsound's internal
    // tree (dsoundi.h->dscommon.h->nt.h), whose SIZE_T/kernel typedefs collide
    // with the title-side bridge every other TU uses. It would need its own
    // driver-env compile (separate bridge + libdsound internal include set).
};

// dmloader: the object loader / container (pchloader.h PCH).
const dmloader_cpp = [_][]const u8{
    D ++ "dmloader/container.cpp",
    D ++ "dmloader/dll.cpp",
    D ++ "dmloader/loader.cpp",
    D ++ "dmloader/object.cpp",
    D ++ "dmloader/stream.cpp",
};

// dmband: band + band-track (pchdmband.h PCH).
const dmband_cpp = [_][]const u8{
    D ++ "dmband/bandinst.cpp",
    D ++ "dmband/bandtrk.cpp",
    D ++ "dmband/dmband.cpp",
    D ++ "dmband/dmbdll.cpp",
    D ++ "dmband/pchmap.cpp",
};

// dmstyle: styles + the style tracks (no PCH; xboxdll UMTYPE).
const dmstyle_cpp = [_][]const u8{
    D ++ "dmstyle/chordtrk.cpp",
    D ++ "dmstyle/cmmdtrk.cpp",
    D ++ "dmstyle/dmsect.cpp",
    D ++ "dmstyle/dmstydll.cpp",
    D ++ "dmstyle/dmstyle.cpp",
    D ++ "dmstyle/motiftrk.cpp",
    D ++ "dmstyle/ptrntrk.cpp",
    D ++ "dmstyle/styletrk.cpp",
    D ++ "dmstyle/audtrk.cpp",
    D ++ "dmstyle/mutetrk.cpp",
    D ++ "dmstyle/dmstyle2.cpp",
    D ++ "dmstyle/filter.cpp",
    D ++ "dmstyle/mgentrk.cpp",
};

// dmcompos: chordmap + composition/template engine (pchcompos.h PCH).
const dmcompos_cpp = [_][]const u8{
    D ++ "dmcompos/dmcmpdll.cpp",
    D ++ "dmcompos/dmcompos.cpp",
    D ++ "dmcompos/dmpers.cpp",
    D ++ "dmcompos/dmtempl.cpp",
    D ++ "dmcompos/spsttrk.cpp",
    D ++ "dmcompos/str.cpp",
    D ++ "dmcompos/perstrk.cpp",
    D ++ "dmcompos/dmcomp2.cpp",
    D ++ "dmcompos/memstm.cpp",
    // RXDK-added: in-lib stub for the OLE32 _ExceptionContext() accessor the
    // sem.hxx/except.hxx CTry machinery references (see the file).
    D ++ "dmcompos/except_stub_rxdk.cpp",
};

// dmime: the performance / segment / audiopath / track engine (pchime.h PCH).
const dmime_cpp = [_][]const u8{
    D ++ "dmime/alist.cpp",
    D ++ "dmime/dmgraph.cpp",
    D ++ "dmime/debug.cpp",
    D ++ "dmime/dmperf.cpp",
    D ++ "dmime/dmprfdll.cpp",
    D ++ "dmime/dmsegobj.cpp",
    D ++ "dmime/dmsstobj.cpp",
    D ++ "dmime/queue.cpp",
    D ++ "dmime/seqtrack.cpp",
    D ++ "dmime/sysextrk.cpp",
    D ++ "dmime/tempotrk.cpp",
    D ++ "dmime/tsigtrk.cpp",
    D ++ "dmime/dmstrm.cpp",
    D ++ "dmime/audpath.cpp",
    D ++ "dmime/lyrictrk.cpp",
    D ++ "dmime/marktrk.cpp",
    D ++ "dmime/paramtrk.cpp",
    D ++ "dmime/segtrtrk.cpp",
    D ++ "dmime/smartref.cpp",
    D ++ "dmime/song.cpp",
    D ++ "dmime/trackhelp.cpp",
    D ++ "dmime/wavtrack.cpp",
    // NOTE: phoneyds.cpp (the CPhoneyDSound software-DirectSound emulator) is
    // EXCLUDED. Under XMIX (Xbox hardware mixing) CPhoneyDSound is never
    // instantiated -- nothing in the lib references it (audpath drives the real
    // DirectSound), so phoneyds.o is dead code never pulled at title link. It
    // also cannot be emitted cleanly here: RXDK's public dsound.h declares
    // IDirectSound in the MIDL C-compatible style (an lpVtbl member + inline
    // forwarders), not the pure-virtual DECLARE_INTERFACE style CPhoneyDSound's
    // overrides assume, so clang never emits its vtable. CAudioSink/CBuffer (the
    // audiosink.cpp/buffer.cpp the audiopath DOES use) stay in the build.
    D ++ "dmime/dowork.cpp",
    D ++ "dmime/dmcreate.cpp",
    D ++ "dmime/CMixBins.cpp",
    // RXDK-added (not in the leak's dmime SOURCES, which resolved these from
    // dsound): the CAudioSink/CBuffer software-DirectSound sink that phoneyds.cpp
    // (the CPhoneyDSound software mixer, vestigial under XMIX) references. Adding
    // them keeps libdmusic self-sufficient instead of leaving those undefined.
    D ++ "dmime/audiosink.cpp",
    D ++ "dmime/buffer.cpp",
};

// dmime C file (MIDL-generated IMediaParams IIDs).
const dmime_c = [_][]const u8{
    D ++ "dmime/medparam_i.c",
};

// dmscript: the interactive-music scripting engine (pchscript.h PCH). COM
// Automation heavy (IActiveScript/IDispatch/VARIANT); uses C++ exceptions + STL.
const dmscript_cpp = [_][]const u8{
    D ++ "dmscript/activescript.cpp",
    D ++ "dmscript/autaudiopath.cpp",
    D ++ "dmscript/autaudiopathconfig.cpp",
    D ++ "dmscript/autconstants.cpp",
    D ++ "dmscript/authelper.cpp",
    D ++ "dmscript/autperformance.cpp",
    D ++ "dmscript/autsegment.cpp",
    D ++ "dmscript/autsegmentstate.cpp",
    D ++ "dmscript/autsong.cpp",
    D ++ "dmscript/containerdisp.cpp",
    D ++ "dmscript/dll_dmscript.cpp",
    D ++ "dmscript/dmscript.cpp",
    D ++ "dmscript/engdisp.cpp",
    D ++ "dmscript/engerror.cpp",
    D ++ "dmscript/engexec.cpp",
    D ++ "dmscript/engexpr.cpp",
    D ++ "dmscript/enginc.cpp",
    D ++ "dmscript/engine.cpp",
    D ++ "dmscript/englex.cpp",
    D ++ "dmscript/englookup.cpp",
    D ++ "dmscript/engparse.cpp",
    D ++ "dmscript/globaldisp.cpp",
    D ++ "dmscript/oleaut.cpp",
    D ++ "dmscript/packexception.cpp",
    D ++ "dmscript/sourcetext.cpp",
    D ++ "dmscript/track.cpp",
    D ++ "dmscript/trackshared.cpp",
    D ++ "dmscript/unkhelp.cpp",
};

pub const slices = [_]Slice{
    .{ .name = "dmsynth", .is_cpp = true, .sources = &dmsynth_cpp, .extra_defines = &.{
        "-DREVERB_ENABLED", "-D_DMUSIC_USER_MODE_", "-DXMIX",
    } },
    .{ .name = "dmloader", .is_cpp = true, .sources = &dmloader_cpp, .pch = D ++ "dmloader/pchloader.h" },
    .{ .name = "dmband", .is_cpp = true, .sources = &dmband_cpp, .pch = D ++ "dmband/pchdmband.h" },
    .{ .name = "dmstyle", .is_cpp = true, .sources = &dmstyle_cpp },
    .{ .name = "dmcompos", .is_cpp = true, .sources = &dmcompos_cpp, .pch = D ++ "dmcompos/pchcompos.h" },
    .{ .name = "dmime", .is_cpp = true, .sources = &dmime_cpp, .pch = D ++ "dmime/pchime.h", .extra_defines = &.{"-DXMIX"} },
    .{ .name = "dmime-c", .is_cpp = false, .sources = &dmime_c },
    // dmguids is THE INITGUID translation unit: -DINITGUID (compile-wide, so it
    // is set before the bridge's <guiddef.h>) flips DEFINE_GUID to its allocating
    // form for this TU only, giving every DirectMusic class/interface/param GUID
    // pulled through pchime real storage here (all other TUs see the extern form).
    .{ .name = "dmguids", .is_cpp = true, .sources = &dmguids_cpp, .extra_defines = &.{ "-DINITGUID", "-DXMIX" } },
    // dmscript: the interactive-music scripting engine. Its OLE-Automation
    // interface surface (complete IDispatch/ITypeInfo via inc/oaidl.h, the
    // IActiveScript* family via the stripped inc/activscp.h) is shimmed as
    // declarations only -- Xbox has no VBScript runtime, so this compiles+links
    // like the rest of the lib but does not provide a functional script engine.
    .{ .name = "dmscript", .is_cpp = true, .sources = &dmscript_cpp, .pch = D ++ "dmscript/pchscript.h", .extra_defines = &.{"-DDMS_NEVER_USE_OLEAUT"} },
};
