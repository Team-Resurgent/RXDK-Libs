//------------------------------------------------------------------------------
// XNet bring-up sample -- RXDK libxnetx (the newer private/ntos/net stack).
//
// Uses the public XNetStartup API (the same one PrometheOS/titles use). The
// dev-kit NIC is owned by the debug monitor (xbdm); this stack's native
// CXbdmClient shares it. XNetStartup is non-blocking -- we then BUSY-POLL
// XNetGetTitleXnAddr until an address is configured, keeping the thread runnable
// so the timer + NIC DPCs keep running. (A blocking wait starves DPCs on the kit;
// PrometheOS likewise polls from its render loop. See [[libxnet-newstack-upgrade]].)
//------------------------------------------------------------------------------

#include "common.h"   // xapi_smoke_trace_line + boot/trace helpers
#include <stdio.h>

// Public XNet API (newer stack). WSAAPI == __stdcall on Xbox; declared manually
// (the title-side winsockx.h needs the full Windows header env to include here).
typedef struct {
    unsigned long  ina;          // IP address (network byte order; 0 if none)
    unsigned long  inaOnline;
    unsigned short wPortOnline;
    unsigned char  abEnet[6];    // Ethernet MAC
    unsigned char  abOnline[20];
} XNADDR;                        // 36 bytes (matches winsockx.h)

// XNetStartupParams: 12 packed BYTEs (winsockx.h). Any field left 0 takes the
// stack's default; we only need cfgSizeOfStruct + cfgFlags.
typedef struct {
    unsigned char cfgSizeOfStruct;
    unsigned char cfgFlags;
    unsigned char cfgPrivatePoolSizeInPages;
    unsigned char cfgEnetReceiveQueueLength;
    unsigned char cfgIpFragMaxSimultaneous;
    unsigned char cfgIpFragMaxPacketDiv256;
    unsigned char cfgSockMaxSockets;
    unsigned char cfgSockDefaultRecvBufsizeInK;
    unsigned char cfgSockDefaultSendBufsizeInK;
    unsigned char cfgKeyRegMax;
    unsigned char cfgSecRegMax;
    unsigned char cfgQosDataLimitDiv4;
} XNetStartupParams;            // 12 bytes

// Devkit-only: allow insecure comms to untrusted hosts (e.g. a PC). REQUIRED on
// the INSECURE library -- without it DhcpConfig parks at FLAG_ACTIVE_NOADDR
// (XNADDR_ETHERNET) to mimic xnets.lib and never acquires an IP. (PrometheOS sets
// this too. See [[libxnet-newstack-upgrade]].)
#define XNET_STARTUP_BYPASS_SECURITY  0x01

extern int           __stdcall XNetStartup(const XNetStartupParams *pxnsp);
extern int           __stdcall XNetCleanup(void);
extern unsigned long __stdcall XNetGetTitleXnAddr(XNADDR *pxna);  // -> XNADDR_* flags
extern unsigned long __stdcall XNetGetEthernetLinkStatus(void);
extern void          __stdcall KeStallExecutionProcessor(unsigned long microseconds);

// XNetGetTitleXnAddr result flags (winsockx.h XNET_GET_XNADDR_*).
#define XNADDR_PENDING   0x00
#define XNADDR_NONE      0x01
#define XNADDR_ETHERNET  0x02
#define XNADDR_STATIC    0x04
#define XNADDR_DHCP      0x08
#define XNADDR_AUTO      0x10

// Ethernet link-status flags (winsockx.h XNET_ETHERNET_LINK_*).
#define LINK_ACTIVE   0x01
#define LINK_100MBPS  0x02
#define LINK_10MBPS   0x04
#define LINK_FULL     0x08
#define LINK_HALF     0x10

int main(void)
{
    int           rc, i;
    XNADDR        xna = {0};
    unsigned long st = XNADDR_PENDING, link;
    unsigned char *o;
    XNetStartupParams xnsp = {0};

    xnsp.cfgSizeOfStruct = sizeof(xnsp);                 // must equal sizeof(XNetStartupParams)
    xnsp.cfgFlags        = XNET_STARTUP_BYPASS_SECURITY; // devkit insecure mode -> DHCP runs

    xapi_smoke_trace_line("xnet-net start");

    DbgPrint("xnet-net: XNetStartup (newer private/ntos/net stack)...\n");
    rc = XNetStartup(&xnsp);
    DbgPrint("xnet-net: XNetStartup returned %d\n", rc);

    // Busy-poll for a configured address (DO NOT block: a blocking wait starves
    // DPCs on the dev kit, so the timer/NIC DPCs that drive DHCP would never run).
    for (i = 0; i < 300; i++) {              // ~30s
        KeStallExecutionProcessor(100000);   // 100ms busy wait keeps DPCs alive
        st = XNetGetTitleXnAddr(&xna);
        if (st & (XNADDR_DHCP | XNADDR_STATIC))
            break;                           // got a real lease / static IP
        if ((i % 10) == 9)
            DbgPrint("xnet-net: ...polling (t=%ds, xnaddr-flags=0x%lx)\n",
                     (i + 1) / 10, st);
    }

    link = XNetGetEthernetLinkStatus();
    DbgPrint("xnet-net: link=0x%02lx [%s %s %s]\n", link,
             (link & LINK_ACTIVE) ? "UP" : "DOWN",
             (link & LINK_100MBPS) ? "100Mbps" : ((link & LINK_10MBPS) ? "10Mbps" : "?"),
             (link & LINK_FULL) ? "full-duplex" : ((link & LINK_HALF) ? "half-duplex" : "?"));

    o = (unsigned char *)&xna.ina;   // network byte order
    DbgPrint("xnet-net: flags=0x%lx IP=%u.%u.%u.%u mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
             st, o[0], o[1], o[2], o[3],
             xna.abEnet[0], xna.abEnet[1], xna.abEnet[2],
             xna.abEnet[3], xna.abEnet[4], xna.abEnet[5]);

    if (st & (XNADDR_DHCP | XNADDR_STATIC))
        DbgPrint("xnet-net: *** network up (address acquired) ***\n");
    else
        DbgPrint("xnet-net: no address (flags=0x%lx)\n", st);

    // Keep the title alive WITHOUT blocking (the DPC-driven stack needs a running
    // thread; a blocking Sleep would freeze it on the kit).
    DbgPrint("xnet-net: idling (busy).\n");
    for (;;) { KeStallExecutionProcessor(100000); }

    return 0;
}
