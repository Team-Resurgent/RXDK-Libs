//------------------------------------------------------------------------------
// XNet bring-up sample -- RXDK libxnet.
//
// Starts the Xbox TCP/IP stack (the nForce MCPX ethernet NIC + PHY), waits for
// DHCP, and reports the ethernet link state and the leased IP address.
//
// The leak's xnet source implements the OLDER public API (XnetInitialize /
// XnetGetEthernetLinkStatus), which the slimmed public winsockx.h does not
// declare (it only has the newer XNetStartup). So we declare the few entry
// points we call here, with their real calling conventions (WSAAPI = __stdcall;
// IpGetBestAddress is an internal cdecl helper that returns the best local IP).
//------------------------------------------------------------------------------

#include "common.h"   // xapi_smoke_trace_line + the boot/trace helpers
#include <stdio.h>

// --- libxnet entry points (see winsock/sockinit.c, ip/iputil.c) --------------
extern int            __stdcall XnetInitialize(const void *initParams, int wait);
extern int            __stdcall XnetCleanup(void);
extern unsigned long  __stdcall XnetGetEthernetLinkStatus(void);
extern unsigned long             IpGetBestAddress(unsigned long *addr); // cdecl, IPADDR* (net order)

// NOTE: KeStallExecutionProcessor (declared in xboxkrnl ke.h) is a kernel busy-wait
// that does NOT block/yield -> the thread stays runnable so timer and NIC DPCs keep
// firing. On the dev kit, a *blocking* wait (KeWaitForSingleObject, e.g. Sleep /
// XnetInitialize(wait=1)) starves DPCs once it's the only thread, so DHCP retries and
// received frames never get processed. We therefore poll with a busy-wait.

// Ethernet link-status flags (winsockx.h XNET_ETHERNET_LINK_*).
#define LINK_ACTIVE       0x01
#define LINK_100MBPS      0x02
#define LINK_10MBPS       0x04
#define LINK_FULL_DUPLEX  0x08
#define LINK_HALF_DUPLEX  0x10

// Address-config flags (rxdk_xnet_compat.h XNET_ADDR_*).
#define ADDR_STATIC       0x04
#define ADDR_DHCP         0x08
#define ADDR_AUTOIP       0x10

static void print_status(void)
{
    unsigned long link = XnetGetEthernetLinkStatus();
    unsigned long ip = 0, flags = IpGetBestAddress(&ip);
    unsigned char *o = (unsigned char *)&ip;  // IPADDR is network byte order

    DbgPrint("xnet-net: link=0x%02x [%s %s %s]\n",
             (unsigned)link,
             (link & LINK_ACTIVE) ? "UP" : "DOWN",
             (link & LINK_100MBPS) ? "100Mbps" : ((link & LINK_10MBPS) ? "10Mbps" : "?"),
             (link & LINK_FULL_DUPLEX) ? "full-duplex" : ((link & LINK_HALF_DUPLEX) ? "half-duplex" : "?"));
    DbgPrint("xnet-net: IP = %u.%u.%u.%u  (addr-flags=0x%x)\n",
             o[0], o[1], o[2], o[3], (unsigned)flags);
}

int main(void)
{
    int           rc;
    unsigned long ip = 0, flags = 0;
    int           i;

    xapi_smoke_trace_line("xnet-net start");

    // Start the stack WITHOUT blocking on DHCP (wait=0): a blocking wait starves
    // DPCs on the kit. We drive DHCP ourselves by busy-polling below.
    DbgPrint("xnet-net: calling XnetInitialize (no-wait; busy-poll for DHCP)...\n");
    rc = XnetInitialize(NULL, 0 /* no wait */);
    DbgPrint("xnet-net: XnetInitialize returned %d\n", rc);

    // Poll up to ~30s for a real lease, keeping the thread runnable (busy-wait) so
    // the TCP fast-timer DPC (DHCP retransmits) and the xbdm/NIC receive DPC keep
    // running. (We must NOT block: a blocking wait starves DPCs on the dev kit.)
    //
    // KNOWN LIMITATION (dev kit): the timer-driven path through the old-stack ->
    // xbdm CXbdmServer bridge hangs the kit at DISPATCH. Being replaced by a port
    // of the newer private/ntos/net stack (XNetStartup), whose CXbdmClient is
    // native to the xbdm protocol. See [[libxnet-port]] memory.
    for (i = 0; i < 300; i++) {
        KeStallExecutionProcessor(100000);   // 100ms busy wait (DPCs stay alive)
        flags = IpGetBestAddress(&ip);
        if (ip && (flags & (ADDR_DHCP | ADDR_STATIC)))
            break;                            // got a real (non-AutoIP) address
        if ((i % 10) == 9)                    // ~ every second
            DbgPrint("xnet-net: ...polling DHCP (t=%ds, ip-flags=0x%x)\n",
                     (i + 1) / 10, (unsigned)flags);
    }

    if (ip && (flags & (ADDR_DHCP | ADDR_STATIC)))
        DbgPrint("xnet-net: *** DHCP lease acquired ***\n");
    else
        DbgPrint("xnet-net: no DHCP lease (flags=0x%x); reporting current state\n",
                 (unsigned)flags);
    print_status();

    // Keep the title alive WITHOUT blocking, so networking (DPC-driven) keeps
    // running. (A blocking Sleep would freeze the stack on the kit.)
    DbgPrint("xnet-net: network up; idling (busy).\n");
    for (;;) { KeStallExecutionProcessor(100000); }

    return 0;
}
