// libxnet source manifest -- the Xbox XNet TCP/IP network stack ported from the
// May-2020 leak (private/ntos/xnet). Custom Microsoft stack (NOT lwIP): phy +
// lib + enet (ARP + nForce/i82558 NIC) + ip (IP/ICMP/IGMP/route/loopback) +
// dhcp + tcp (TCP/UDP/PCB/timers) + dns + winsock (sockets + XnetInitialize) +
// http. ppp/modem (dial-up) and test/ are OUT of scope. Mirrors libdsound's
// per-subdir slice layout; http/*.cxx are C++, everything else is C.
//
// Kernel-runtime component (runs at DISPATCH_LEVEL, pokes the MCPX NIC) -- see
// site/bridge_xnet.h (force-included before each TU).

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
};

const X = "libs/libxnet";

pub const c_sources = [_][]const u8{
    // phy: Ethernet PHY transceiver (MII, link negotiation)
    X ++ "/phy/phy.c",
    // lib: packet pools + net utilities (+ C reimpl of i386/tcpipxsum.asm)
    X ++ "/lib/netpool.c",
    X ++ "/lib/netutil.c",
    X ++ "/lib/tcpipxsum.c",
    // enet: ARP, Ethernet framing, NIC drivers (i82558 debug board + nForce MCPX)
    X ++ "/enet/arp.c",
    X ++ "/enet/enet.c",
    X ++ "/enet/i82558.c",
    X ++ "/enet/xnic.c",
    // ip: IPv4 / ICMP / IGMP / routing / loopback
    X ++ "/ip/ipinit.c",
    X ++ "/ip/iprecv.c",
    X ++ "/ip/ipsend.c",
    X ++ "/ip/icmp.c",
    X ++ "/ip/igmp.c",
    X ++ "/ip/route.c",
    X ++ "/ip/loopback.c",
    X ++ "/ip/iputil.c",
    // dhcp: DHCP client state machine
    X ++ "/dhcp/dhcp.c",
    X ++ "/dhcp/dhcpdump.c",
    // tcp: TCP, UDP, PCBs, multicast, timers
    X ++ "/tcp/tcpinit.c",
    X ++ "/tcp/tcprecv.c",
    X ++ "/tcp/tcpsend.c",
    X ++ "/tcp/tcpconn.c",
    X ++ "/tcp/udp.c",
    X ++ "/tcp/pcb.c",
    X ++ "/tcp/mcast.c",
    // dns: DNS resolver
    X ++ "/dns/dns.c",
    // winsock: socket API + XnetInitialize / XNetCleanup
    X ++ "/winsock/sockinit.c",
    X ++ "/winsock/socket.c",
    X ++ "/winsock/sockopt.c",
    X ++ "/winsock/recv.c",
    X ++ "/winsock/send.c",
    X ++ "/winsock/connect.c",
    X ++ "/winsock/select.c",
    X ++ "/winsock/sockmisc.c",
    X ++ "/winsock/enumprot.c",
    X ++ "/winsock/getxbyy.c",
};

// http: lightweight HTTP client (C++). DEFERRED -- it pulls wininet (wininetp.h:
// HttpSendRequestA, LPWIN32_FIND_DATAW) and its own Free/MAlloc heap macros, and
// isn't needed for network bring-up (XnetInitialize + sockets + DHCP). Re-enable
// once the core stack works on hardware.
pub const cpp_sources = [_][]const u8{
    // X ++ "/http/handle.cxx",
    // X ++ "/http/http.cxx",
};

pub const slices = [_]Slice{
    .{ .name = "xnet", .is_cpp = false, .sources = &c_sources },
};
