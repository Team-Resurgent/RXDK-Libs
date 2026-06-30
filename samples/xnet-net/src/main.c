//------------------------------------------------------------------------------
// XNet bring-up sample -- RXDK libxnet (the private/ntos/net stack).
//
// Uses the public XNetStartup API (the same one PrometheOS/titles use). The
// dev-kit NIC is owned by the debug monitor (xbdm); this stack's native
// CXbdmClient shares it. XNetStartup is non-blocking -- we then BUSY-POLL
// XNetGetTitleXnAddr until an address is configured, keeping the thread runnable
// so the timer + NIC DPCs keep running. (A blocking wait starves DPCs on the kit;
// PrometheOS likewise polls from its render loop. See [[libxnet-newstack-upgrade]].)
//
// Once a DHCP/static address is up, it hosts a single web page over the libxnet
// BSD socket API: bind/listen on :80 and serve a small HTML page to any browser.
// Every socket is non-blocking + busy-polled for the same DPC reason, so the
// server loop also keeps the stack alive. Point a browser at the kit's IP.
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

// ---------------------------------------------------------------------------
// Berkeley/Winsock socket API (exported by libxnet). winsockx.h pulls the full
// <windows.h> environment, so -- like the XNet decls above -- declare just what
// the HTTP server needs by hand. WSAAPI == __stdcall on Xbox.
// ---------------------------------------------------------------------------
typedef unsigned int SOCKET;                  // UINT_PTR (32-bit Xbox)

struct in_addr     { unsigned long s_addr; };
struct sockaddr_in {
    short           sin_family;
    unsigned short  sin_port;                 // network byte order
    struct in_addr  sin_addr;
    char            sin_zero[8];
};
struct sockaddr    { unsigned short sa_family; char sa_data[14]; };

#define AF_INET         2
#define SOCK_STREAM     1
#define IPPROTO_TCP     6
#define INADDR_ANY      0
#define INVALID_SOCKET  ((SOCKET)~0)
#define SOCKET_ERROR    (-1)
#define SOL_SOCKET      0xffff
#define SO_REUSEADDR    0x0004
#define FIONBIO         0x8004667EL           // _IOW('f',126,u_long): set non-blocking
#define WSAEWOULDBLOCK  10035

// WSAStartup bumps a SEPARATE socket refcount inside the stack; socket() returns
// WSANOTINITIALISED (10093) without it. XNetStartup only bumps the XNet refcount,
// so the standard Xbox order is XNetStartup THEN WSAStartup. (32-bit WSADATA layout.)
typedef struct WSAData {
    unsigned short wVersion, wHighVersion;
    char           szDescription[257];
    char           szSystemStatus[129];
    unsigned short iMaxSockets, iMaxUdpDg;
    char          *lpVendorInfo;
} WSADATA;
#define MAKEWORD(lo,hi) ((unsigned short)(((unsigned char)(lo)) | ((unsigned short)(unsigned char)(hi) << 8)))
extern int            __stdcall WSAStartup(unsigned short wVersionRequired, WSADATA *lpWSAData);

extern SOCKET         __stdcall socket(int af, int type, int protocol);
extern int            __stdcall closesocket(SOCKET s);
extern int            __stdcall bind(SOCKET s, const struct sockaddr *name, int namelen);
extern int            __stdcall listen(SOCKET s, int backlog);
extern SOCKET         __stdcall accept(SOCKET s, struct sockaddr *addr, int *addrlen);
extern int            __stdcall recv(SOCKET s, char *buf, int len, int flags);
extern int            __stdcall send(SOCKET s, const char *buf, int len, int flags);
extern int            __stdcall setsockopt(SOCKET s, int level, int optname,
                                           const char *optval, int optlen);
extern int            __stdcall ioctlsocket(SOCKET s, long cmd, unsigned long *argp);
extern unsigned short __stdcall htons(unsigned short hostshort);
extern int            __stdcall WSAGetLastError(void);

// ---------------------------------------------------------------------------
// Tiny single-page HTTP/1.0 server.
//
// The XNet stack is DPC-driven and a BLOCKING socket call would starve those
// DPCs on the kit (same reason the address poll busy-waits), so every socket is
// non-blocking and we spin with KeStallExecutionProcessor between would-block
// retries -- that keeps the thread runnable so the NIC/timer DPCs keep pumping.
// ---------------------------------------------------------------------------

static const char g_page[] =
    "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
    "<title>RXDK Xbox</title></head>"
    "<body style=\"font-family:Segoe UI,sans-serif;text-align:center;"
    "background:#107C10;color:#fff;padding-top:12%\">"
    "<h1>&#x2714; Hello from the Xbox</h1>"
    "<p>This page is served by <b>libxnet</b> running on an original Xbox dev kit.</p>"
    "</body></html>";

// Send the whole buffer, polling past WSAEWOULDBLOCK so we never block the stack.
static int send_all(SOCKET s, const char *buf, int len)
{
    int off = 0, n, idle = 0;
    while (off < len) {
        n = send(s, buf + off, len - off, 0);
        if (n > 0) { off += n; idle = 0; continue; }
        if (WSAGetLastError() != WSAEWOULDBLOCK)
            return -1;                       // real error
        if (++idle > 2000)
            return -1;                       // ~4s with nothing drained -> give up
        KeStallExecutionProcessor(2000);     // 2ms; lets the TX DPC drain the socket
    }
    return 0;
}

// Read and discard the request (we serve the same page for any GET), then write
// the response and close. Returns when the exchange is done.
static void handle_client(SOCKET cli)
{
    unsigned long nb = 1;
    char  req[512], hdr[160];
    int   n, idle = 0, got = 0, hlen;

    ioctlsocket(cli, FIONBIO, &nb);

    // Drain the request: read until we've seen the end-of-headers blank line, the
    // peer stalls, or a small cap -- enough to clear the RX buffer for any GET.
    while (got < 8192) {
        n = recv(cli, req, sizeof(req) - 1, 0);
        if (n > 0) {
            req[n] = 0; got += n; idle = 0;
            if (got >= 4 && (req[n-1] == '\n'))   // crude end-of-headers heuristic
                break;
        } else if (n == 0) {
            break;                            // peer closed
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) break;
            if (++idle > 200) break;          // ~1s with no request -> just respond
            KeStallExecutionProcessor(5000);  // 5ms
        }
    }

    hlen = sprintf(hdr,
                   "HTTP/1.0 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: %d\r\n"
                   "Connection: close\r\n\r\n",
                   (int)(sizeof(g_page) - 1));

    if (send_all(cli, hdr, hlen) == 0)
        send_all(cli, g_page, (int)(sizeof(g_page) - 1));
}

// Listen on :80 and serve g_page to every connection. Never returns.
static void serve_http(unsigned long ip_be)
{
    SOCKET srv, cli;
    struct sockaddr_in addr;
    unsigned long nb = 1;
    int yes = 1;
    unsigned char *o = (unsigned char *)&ip_be;
    DWORD hits = 0;

    srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv == INVALID_SOCKET) {
        DbgPrint("http: socket() failed (err=%d)\n", WSAGetLastError());
        return;
    }
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(80);
    addr.sin_addr.s_addr = INADDR_ANY;
    { int i; for (i = 0; i < (int)sizeof(addr.sin_zero); i++) addr.sin_zero[i] = 0; }

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        DbgPrint("http: bind(:80) failed (err=%d)\n", WSAGetLastError());
        closesocket(srv);
        return;
    }
    if (listen(srv, 4) == SOCKET_ERROR) {
        DbgPrint("http: listen() failed (err=%d)\n", WSAGetLastError());
        closesocket(srv);
        return;
    }
    ioctlsocket(srv, FIONBIO, &nb);   // non-blocking accept (don't starve DPCs)

    DbgPrint("http: serving on http://%u.%u.%u.%u/  -- open it in a browser\n",
             o[0], o[1], o[2], o[3]);

    for (;;) {
        cli = accept(srv, NULL, NULL);
        if (cli == INVALID_SOCKET) {
            KeStallExecutionProcessor(10000);  // 10ms; poll again, keep DPCs alive
            continue;
        }
        handle_client(cli);
        closesocket(cli);
        DbgPrint("http: served request #%lu\n", (unsigned long)++hits);
    }
}

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

    if (st & (XNADDR_DHCP | XNADDR_STATIC)) {
        WSADATA wsa;
        int wrc;
        DbgPrint("xnet-net: *** network up (address acquired) ***\n");
        // socket() needs WSAStartup (separate refcount from XNetStartup).
        wrc = WSAStartup(MAKEWORD(2, 2), &wsa);
        DbgPrint("xnet-net: WSAStartup returned %d\n", wrc);
        // Host a single web page. serve_http never returns (busy-polling accept),
        // which also keeps the DPC-driven stack alive.
        serve_http(xna.ina);
    } else {
        DbgPrint("xnet-net: no address (flags=0x%lx)\n", st);
    }

    // Fallback (no address): keep the title alive WITHOUT blocking (the DPC-driven
    // stack needs a running thread; a blocking Sleep would freeze it on the kit).
    DbgPrint("xnet-net: idling (busy).\n");
    for (;;) { KeStallExecutionProcessor(100000); }

    return 0;
}
