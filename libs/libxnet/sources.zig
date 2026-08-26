// libxnet source manifest -- the Xbox XNet stack ported from the May-2020 leak
// (private/ntos/net). This is the public XNetStartup-API stack the kit's xbdm was
// built against (native CXbdmServer/CXbdmClient dev-kit NIC sharing). Build as
// XNET_BUILD_LIBX (the vendor LIBTAG=X / xnet.lib variant):
//   features ARP/DHCP/DNS/FRAG/ICMP/INSECURE/ROUTE/XBDM_CLIENT/XBOX
//   (XBDM_CLIENT = native dev-kit bridge; INSECURE = no crypto dependency).
//
// C++ throughout (classes/vtables); tcpipxsum is our C reimpl of i386/tcpipxsum.asm.
// Precompiled header is net/xnp.h (force-included via site/bridge_xnet.h).
// See [[libxnet-newstack-upgrade]] memory for the full plan.

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
};

const X = "libs/libxnet";

// C reimpl of i386/tcpipxsum.asm (RFC1071 internet checksum).
pub const c_sources = [_][]const u8{
    X ++ "/lib/tcpipxsum.c",
};

// The newer net stack (private/ntos/net sources.inc SOURCES list, minus the .asm).
pub const cpp_sources = [_][]const u8{
    X ++ "/net/base.cpp",
    X ++ "/net/enet.cpp",
    X ++ "/net/halw.cpp",
    X ++ "/net/halx.cpp",
    X ++ "/net/ip.cpp",
    X ++ "/net/ipdhcp.cpp",
    X ++ "/net/ipdns.cpp",
    X ++ "/net/ipicmp.cpp",
    X ++ "/net/ipqos.cpp",
    X ++ "/net/nicw.cpp",
    X ++ "/net/nicx.cpp",
    X ++ "/net/sock.cpp",
    X ++ "/net/socktcp.cpp",
    X ++ "/net/sockudp.cpp",
    X ++ "/net/xnet.cpp",
    X ++ "/net/xnetp.cpp",
};

pub const slices = [_]Slice{
    .{ .name = "xnet-c", .is_cpp = false, .sources = &c_sources },
    .{ .name = "xnet-cpp", .is_cpp = true, .sources = &cpp_sources },
};
