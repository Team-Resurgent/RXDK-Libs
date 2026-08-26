// libxonline source manifest -- the Xbox Live client library (xonline.lib) ported
// from the May-2020 leak (private/online). This is the TITLE-SIDE Xbox Live client:
// logon/presence/accounts/billing/match/stats/users/service/msgclient, content
// download+patching (contdl/contenum/contrm/contutil/contver/download/upload/
// autoupd/difpatch/patchutl/dvdload/localcache/cfcache/dirops/baseio), the XDK's
// OWN self-contained crypto (kerberos/krb5/md4ms/msr_md5/msasn1/symmdec/decalign)
// and LZX decompressor (decblk/decin/decout/dectree/decuncmp/decverb/decxlat/
// xonlzx/maketbl), plus utilities (xbosutil/xrlutil/xontask/olddash/xonline).
//
// The EXACT set is the leak's private/online/sources.inc SOURCES= list, built as
// LIBTAG=X (-DXONLINE_BUILD_LIBX -> xonline.lib, the retail title variant). Every
// .cpp #includes the precompiled header "xonp.h" (staged in inc/) + "xonver.h".
// Precompiled header is force-included via site/bridge_xonline.h.
//
// xonline sits on top of RXDK's libxnet (winsock/XNet), libkernel, libxapi and
// libc. The crypto and LZX code are the XDK's own self-contained implementations
// (no OpenSSL/external crypto) -- ported verbatim. See [[xonline-rxdk-port]].
//
// C++ throughout (the CXo client class + member-pointer task dispatch), so like
// libxnet it is built with clang's MSVC C++ ABI so member-ptr/vtable/struct
// layouts match. Xbox Live servers are long dead: this only needs to compile +
// link, never run.

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
};

const X = "libs/libxonline";

// The exact SOURCES= list from private/online/sources.inc (LIBTAG=X / xonline.lib).
pub const cpp_sources = [_][]const u8{
    X ++ "/src/stats.cpp",
    X ++ "/src/xonexports.cpp",
    X ++ "/src/xonline.cpp",
    X ++ "/src/kerberos.cpp",
    X ++ "/src/krb5.cpp",
    X ++ "/src/msr_md5.cpp",
    X ++ "/src/md4ms.cpp",
    X ++ "/src/msasn1.cpp",
    X ++ "/src/autoupd.cpp",
    X ++ "/src/baseio.cpp",
    X ++ "/src/cfcache.cpp",
    X ++ "/src/contutil.cpp",
    X ++ "/src/contdl.cpp",
    X ++ "/src/contrm.cpp",
    X ++ "/src/contver.cpp",
    X ++ "/src/contenum.cpp",
    X ++ "/src/dirops.cpp",
    X ++ "/src/download.cpp",
    X ++ "/src/symmdec.cpp",
    X ++ "/src/upload.cpp",
    X ++ "/src/xontask.cpp",
    X ++ "/src/xrlutil.cpp",
    X ++ "/src/decalign.cpp",
    X ++ "/src/decblk.cpp",
    X ++ "/src/decin.cpp",
    X ++ "/src/decout.cpp",
    X ++ "/src/dectree.cpp",
    X ++ "/src/decuncmp.cpp",
    X ++ "/src/decverb.cpp",
    X ++ "/src/decxlat.cpp",
    X ++ "/src/maketbl.cpp",
    X ++ "/src/xonlzx.cpp",
    X ++ "/src/logon.cpp",
    X ++ "/src/match.cpp",
    X ++ "/src/service.cpp",
    X ++ "/src/users.cpp",
    X ++ "/src/presence.cpp",
    X ++ "/src/billing.cpp",
    X ++ "/src/xbosutil.cpp",
    X ++ "/src/localcache.cpp",
    X ++ "/src/msgclient.cpp",
    X ++ "/src/accounts.cpp",
    X ++ "/src/difpatch.cpp",
    X ++ "/src/patchutl.cpp",
    X ++ "/src/dvdload.cpp",
    X ++ "/src/olddash.cpp",
};

// The UIX drop-in UI. 5849 ships this as its own uix.lib rather than inside
// xonline.lib, so it is packed separately -- see the root build.zig's libuix.
pub const uix_sources = [_][]const u8{
    X ++ "/src/uix5849.cpp",
    X ++ "/src/uix_skin.cpp",
};

pub const slices = [_]Slice{
    .{ .name = "xonline-cpp", .is_cpp = true, .sources = &cpp_sources },
};
