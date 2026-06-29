/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    enet.c

Abstract:

    Handle Ethernet frame transmission and reception

Revision History:

    05/04/2000 davidx
        Created it.

--*/

#include "precomp.h"

//
// Ethernet broadcast address
//
const BYTE EnetBroadcastAddr[ENETADDRLEN] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

//
// IEEE LLC and SNAP headers for an Ethernet frame
//
const BYTE IeeeEnetHeaders[] = {
    SNAP_DSAP,      // DSAP - 0xaa
    SNAP_SSAP,      // SSAP - 0xaa
    LLC_DGRAM,      // control - 3
    0, 0, 0         // org code
};

//
// Maximum length of the transmit and receive queue.
//
UINT cfgXmitQLength = 8;
UINT cfgRecvQLength = 10;

//
// Enet system shutdown notification routine
//
PRIVATE BOOL EnetShutdownNotifyRegistered;
PRIVATE HAL_SHUTDOWN_REGISTRATION EnetShutdownNotification;

//
// RXDK: dev-kit debug-monitor enet sharing (CXbdmClient bridge).
//
// On a dev kit the debug monitor (xbdm.dll) owns the NIC -- DbgPrint runs over
// ethernet. KeGetCurrentPrcb()->DmEnetFunc points to xbdm's CXbdmServer C++
// object (NOT a function -- that is the old private/ntos/xnet hand-off this stack
// was originally written for; the shipped 5849 xbdm uses the newer
// private/ntos/net CXbdmServer/CXbdmClient vtable protocol). We become the title
// "client": read DmEnetFunc as CXbdmServer*, validate its 'XBD6' cookie, call
// InitClient/AttachClient, then transmit via CXbdmServer::Xmit and receive via
// our CXbdmClient::EnetRecv. This keeps debug-over-enet alive while the title
// networks. On retail/xemu DmEnetFunc is NULL and we fall back to owning the NIC
// (NicInitialize). See [[libxnet-port]] memory + nicx.cpp/nicx.h in the leak.
//
PRIVATE BOOL  XbdmClientActive(void);     // TRUE once attached to the xbdm server
PRIVATE void  XbdmClientXmit(Packet* pkt);
PRIVATE BOOL  XbdmClientXmitReady(void);
PRIVATE BOOL  XbdmClientStart(IfEnet* ifp);
PRIVATE void  XbdmClientStop(void);       // DetachClient + clear bridge state


PRIVATE VOID
EnetTransmitPacket(
    IfEnet* ifp,
    Packet* pkt,
    const BYTE* dsthwaddr
    )

/*++

Routine Description:

    Transmit an IP datagram to the Ethernet interface

Arguments:

    ifp - Points to the interface structure
    pkt - Points to the packet to be transmitted
    dsthwaddr - Specifies the destination Ethernet hardware address

Return Value:

    NONE

--*/

{
    EnetFrameHeader* enethdr;

    //
    // We assume that the outgoing packet has enough free
    // space before the data buffer to hold the Ethernet frame header.
    //
    ASSERT(PktSpaceBefore(pkt) >= ENETHDRLEN);

    pkt->data -= ENETHDRLEN;
    pkt->datalen += ENETHDRLEN;

    // Now slap on the Ethernet frame header and send it out
    enethdr = GETPKTDATA(pkt, EnetFrameHeader);
    CopyMem(enethdr->dstaddr, dsthwaddr, ENETADDRLEN);
    CopyMem(enethdr->srcaddr, ifp->hwaddr, ENETADDRLEN);
    
    enethdr->etherType = HTONS(ENETTYPE_IP);

    // On a dev kit, hand the framed packet to the debug monitor's NIC; otherwise
    // drive our own NIC hardware.
    if (XbdmClientActive())
        XbdmClientXmit(pkt);
    else
        NicTransmitFrame(ifp, pkt);
}


VOID
EnetReceiveFrame(
    IfEnet* ifp,
    Packet* pkt
    )

/*++

Routine Description:

    Process a received Ethernet frame

Arguments:

    ifp - Points to the interface structure
    pkt - Points to the received packet

Return Value:

    NONE

--*/

{
    EnetFrameHeader* enethdr;
    WORD etherType;

    // We assume the whole frame fits inside a single Packet
    // and the packet data length is validated by the NIC functions.
    // We also assume the Ethernet frame header starts on
    // a WORD-aligned boundary.
    ASSERT(pkt->datalen >= ENETHDRLEN + ENET_MINDATASIZE &&
           pkt->datalen <= ENETHDRLEN + ENET_MAXDATASIZE);

    // Peel off the Ethernet frame header
    enethdr = GETPKTDATA(pkt, EnetFrameHeader);
    pkt->data += ENETHDRLEN;
    pkt->datalen -= ENETHDRLEN;
    etherType = NTOHS(enethdr->etherType);

    // Peel off the IEEE 802.3 LLC and SNAP headers if necessary
    if (IsIEEE802Frame(etherType)) {
        IEEE802Header* ieeehdr = GETPKTDATA(pkt, IEEE802Header);
        pkt->data += IEEE802HDRLEN;
        pkt->datalen -= IEEE802HDRLEN;

        // If the IEEE frame wasn't meant for us, discard it.
        if (!EqualMem(ieeehdr, IeeeEnetHeaders, IEEE802HDRLEN))
            goto discard;

        etherType = NTOHS(ieeehdr->etherType);
    }

    if (etherType == ENETTYPE_IP) {
        // Was the frame received as a broadcast or multicast?
        if (IsEnetAddrMcast(enethdr->dstaddr))
            pkt->pktflags |= PKTFLAG_MCAST;

        // Pass the received frame up to the IP layer and return
        IPRECEIVEPACKET(pkt);
        return;
    }
    
    if (etherType == ENETTYPE_ARP) {
        if (EqualMem(enethdr->srcaddr, ifp->hwaddr, ENETADDRLEN)) {
            // If an ARP packet was receive and the source address
            // was the same as ours, then we have an Ethernet
            // address conflict. We assume we don't receive
            // our own transmissions.
            WARNING_("Ethernet address conflict: db %x!", enethdr);
            ASSERT(FALSE);
        } else {
            // Process incoming ARP packets
            ArpReceivePacket(ifp, pkt);
            return;
        }
    }

discard:
    // Ethernet frame wasn't recognized; quietly discard it.
    COMPLETEPACKET(pkt, NETERR_DISCARDED);
}


VOID
EnetStartOutput(
    IfEnet* ifp
    )

/*++

Routine Description:

    Start output on the Ethernet interface

Arguments:

    ifp - Points to the interface structure

Return Value:

    NONE

--*/

{
    Packet* pkt;
    ArpCacheEntry* arpEntry;
    NTSTATUS status;
    IPADDR nexthop;

    // Must be called at DISPATCH_LEVEL.
    ASSERT_DISPATCH_LEVEL();

    //
    // Remove the packet at the head of the output queue
    // NOTE: We need to make sure we don't flood the NIC's command queue.
    // 
    while (!PktQIsEmpty(&ifp->sendq) &&
           (XbdmClientActive() ? XbdmClientXmitReady() : !NicIsXmitQFull(ifp))) {
        pkt = IfDequeuePacket(ifp);

        nexthop = pkt->nexthop;
        if (IfBcastAddr(ifp, nexthop)) {
            // Broadcast packet
            // NOTE: We assume broadcast loopback is handled in the upper layer.
            if (IfUp(ifp)) {
                EnetTransmitPacket(ifp, pkt, EnetBroadcastAddr);
                continue;
            }
            status = NETERR_NETDOWN;
        } else if (IS_MCAST_IPADDR(nexthop)) {
            // Multicast packet
            // NOTE: We assume multicast loopback is handled in the upper layer.
            if (IfUp(ifp)) {
                BYTE mcastaddr[ENETADDRLEN];
                EnetGenerateMcastAddr(nexthop, mcastaddr);
                EnetTransmitPacket(ifp, pkt, mcastaddr);
                continue;
            }
            status = NETERR_NETDOWN;
        } else {
            // Unicast packet
            // Otherwise, resolve the nexthop IP address to Ethernet address
            if (!IfRunning(ifp)) {
                // Can't send unicast message if the interface is inactive
                status = NETERR_NETDOWN;
            } else if ((arpEntry = ArpFindCacheEntry(ifp, nexthop, RESOLVE_SEND_REQUEST)) == NULL) {
                status = NETERR_MEMORY;
            } else if (IsArpEntryOk(arpEntry)) {
                // Found a valid existing entry in the ARP cache
                // for the next hop address
                EnetTransmitPacket(ifp, pkt, arpEntry->enetaddr);
                continue;
            } else if (IsArpEntryBad(arpEntry)) {
                // Found an existing entry for the next hop address
                // but the entry indicates the target is unreachable.
                status = NETERR_UNREACHABLE;
            } else {
                // Created a new entry for the next hop address
                // or found an existing entry that's currently being resolved
                ASSERT(IsArpEntryBusy(arpEntry));
                PktQInsertTail(&arpEntry->waitq, pkt);
                continue;
            }
        }

        // Complete the packet with error status and move on
        VERBOSE_("Failed to send packet: 0x%x", status);
        COMPLETEPACKET(pkt, status);
    }
}


PRIVATE VOID
EnetDelete(
    IfEnet* ifp
    )

/*++

Routine Description:

    Delete the Ethernet interface

Arguments:

    ifp - Points to the interface structure

Return Value:

    NONE

--*/

{
    RUNS_AT_DISPATCH_LEVEL

    IFENET_DELETE_CHECK(ifp);
    if (ifp->refcount) return;

    // Unregister shutdown notification routine
    if (EnetShutdownNotifyRegistered) {
        HalRegisterShutdownNotification(&EnetShutdownNotification, FALSE);
        EnetShutdownNotifyRegistered = FALSE;
    }

    // Make sure the NIC is stopped
    BOOL wasBridge = XbdmClientActive();
    ifp->flags &= ~(IFFLAG_UP|IFFLAG_RUNNING);
    if (wasBridge)
        XbdmClientStop();       // detach from xbdm; don't touch the hardware
    else
        NicReset(ifp, TRUE);

    // Cleanup the send queue
    while (!PktQIsEmpty(&ifp->sendq)) {
        Packet* pkt = PktQRemoveHead(&ifp->sendq);
        COMPLETEPACKET(pkt, NETERR_CANCELLED);
    }

    ArpCleanup(ifp);
    if (!wasBridge)
        NicCleanup(ifp);
    SysFree(ifp);
}


PRIVATE VOID
EnetShutdownNotifyProc(
    HAL_SHUTDOWN_REGISTRATION* param
    )

/*++

Routine Description:

    Notification routine that's called during system shutdown

Arguments:

    param - Shutdown notification parameters

Return Value:

    NONE

--*/

{
    KIRQL irql = RaiseToDpc();
    IfEnet* ifp = (IfEnet*) LanIfp;

    // Make sure the NIC is stopped
    ifp->flags &= ~(IFFLAG_UP|IFFLAG_RUNNING);
    NicReset(ifp, TRUE);

    LowerFromDpc(irql);
}


PRIVATE VOID
EnetTimer(
    IfEnet* ifp
    )

/*++

Routine Description:

    Ethernet interface timer routine (called once a second)

Arguments:

    ifp - Points to the interface structure

Return Value:

    NONE

--*/

{
    if (IfUp(ifp)) {
        ArpTimerProc(ifp);
        // Bridge mode: xbdm services the NIC hardware timer/poll, not us.
        if (!XbdmClientActive())
            NicTimerProc(ifp);
    }
}


PRIVATE NTSTATUS
EnetSetMcastAddrs(
    IfEnet* ifp
    )

/*++

Routine Description:

    Update the multicast filter on the NIC hardware

Arguments:

    ifp - Points to the interface structure

Return Value:

    Status code

--*/

{
    BYTE* addrs;
    UINT count;
    NTSTATUS status;

    RUNS_AT_DISPATCH_LEVEL

    // Bridge mode: xbdm owns the NIC and its multicast filter (it forwards
    // broadcast/multicast frames to us via EnetRecv). Touching the hardware
    // filter here hangs/corrupts xbdm's NIC, so skip it.
    if (XbdmClientActive())
        return NETERR_OK;

    count = ifp->mcastData->groupCount;
    if (count) {
        addrs = SysAlloc(count * ENETADDRLEN, PTAG_MCAST);
        if (addrs) {
            BYTE* p = addrs;
            IfMcastGroup* mcastgrp = ifp->mcastData->mcastGroups;
            UINT i;

            for (i=0; i < count; i++) {
                EnetGenerateMcastAddr(mcastgrp->mcastaddr, p);
                mcastgrp++;
                p += ENETADDRLEN;
            }
        } else {
            return NETERR_MEMORY;
        }
    } else {
        addrs = NULL;
    }

    status = NicSetMcastAddrs(ifp, addrs, count);
    SysFree(addrs);
    return status;
}


PRIVATE NTSTATUS
EnetIoctl(
    IfEnet* ifp,
    INT ctlcode,
    VOID* inbuf,
    UINT inlen,
    VOID* outbuf,
    UINT* outlen
    )

/*++

Routine Description:

    Ethernet interface I/O control function

Arguments:

    ifp - Points to the interface structure
    ctlcode - Control code
    inbuf - Points to the input buffer
    inlen - Size of the input buffer
    outbuf - Points to the output buffer
    outlen - On entry, this contains the size of the output buffer
        On return, this is the actually number of output bytes

Return Value:

    Status code

--*/

{
    NTSTATUS status;
    IPADDR ipaddr;
    KIRQL irql = RaiseToDpc();

    switch (ctlcode) {
    case IFCTL_SET_MCAST_ADDRS:
        //
        // Send multicast addresses down to the NIC
        //
        status = EnetSetMcastAddrs(ifp);
        break;

    case IFCTL_CHECK_ADDR_CONFLICT:
        //
        // Check for IP address conflict
        //
        ASSERT(inlen == IPADDRLEN);

        ipaddr = *((IPADDR*) inbuf);
        ASSERT(XnetIsValidUnicastAddr(ipaddr));
        
        ifp->checkConflictAddr = ipaddr;
        status = ipaddr ? ArpSendRequest(ifp, ipaddr, ipaddr) : NETERR_OK;
        break;

    default:
        status = NETERR_NOTIMPL;
        break;
    }

    LowerFromDpc(irql);
    return status;
}


//========================================================================
// RXDK: CXbdmClient bridge to the dev-kit debug monitor (xbdm) enet server
//========================================================================
//
// xbdm's CXbdmServer/CXbdmClient are MSVC C++ objects. We talk to them with
// hand-built vtables and __thiscall function pointers (this in ECX) so the ABI
// matches MSVC regardless of our clang x86-windows-gnu C++ ABI. We only ever
// reference these objects through their vtable slots, so name mangling is
// irrelevant; only the slot order and __thiscall convention matter.
//
// CXbdmServer vtable (nicx.h, single inheritance):
//   [0] NicStop      [1] InitClient   [2] AttachClient  [3] DetachClient
//   [4] GetXnAddr    [5] Xmit         [6] XmitReady
// followed by data member DWORD _dwCookie (at offset sizeof(vptr)).
//
// CXbdmClient vtable (the object WE hand to the server):
//   [0] XmitComplete [1] EnetRecv     [2] EnetPush      [3] GetXnAddr
//
#define XBDM_SERVER_COOKIE 'XBD6'   // CXbdmServer::IsValidServer()

// XNADDRXBDM (nicx.h): we only touch abEnet@2 and ina@8.
typedef struct _RxdkXnAddrXbdm {
    BYTE  bSizeOfStruct;
    BYTE  bFlags;
    BYTE  abEnet[6];
    ULONG ina;
    ULONG inaOnline;
    WORD  wPortOnline;
    BYTE  abIdOnline[20];
} RxdkXnAddrXbdm;

// CXbdmServer method pointer types (CEnetAddr* is just a 6-byte buffer).
typedef void     (__thiscall *RxdkFnNicStop)(void* self);
typedef NTSTATUS (__thiscall *RxdkFnInitClient)(void* self, UINT recvQ, UINT xmitQ, BYTE* pea, BOOL* pfLinkUp);
typedef void     (__thiscall *RxdkFnAttachClient)(void* self, void* pClient);
typedef void     (__thiscall *RxdkFnDetachClient)(void* self);
typedef DWORD    (__thiscall *RxdkFnGetXnAddr)(void* self, RxdkXnAddrXbdm* px);
typedef void     (__thiscall *RxdkFnXmit)(void* self, void* pvPkt, void* pv, UINT cb);
typedef BOOL     (__thiscall *RxdkFnXmitReady)(void* self);

#define RXDK_SRV_VTBL(p)   (*(void***)(p))
#define RXDK_SRV_COOKIE(p) (*(DWORD*)((BYTE*)(p) + sizeof(void*)))

// Our CXbdmClient: first member MUST be the vtable pointer the server derefs.
typedef struct _RxdkXbdmClient {
    void** vtbl;
} RxdkXbdmClient;

PRIVATE void* g_pXbdmServer;   // CXbdmServer* once attached, else NULL
PRIVATE IfEnet* g_XbdmIfp;     // the shared enet interface

// --- CXbdmClient callbacks (called by xbdm at DISPATCH_LEVEL) ----------------

static void __thiscall RxdkCli_XmitComplete(void* self, void* pvPkt) {
    UNREFERENCED_PARAMETER(self);
    // Runs at DISPATCH from xbdm's NIC DPC -- no DbgPrint (see XbdmClientXmit note).
    // xbdm finished sending the frame we queued; complete our packet.
    XnetCompletePacket((Packet*) pvPkt, NETERR_OK);
}

static void __thiscall RxdkCli_EnetRecv(void* self, UINT uiFlags, void* pv, UINT cb, UINT uiType) {
    // pv = payload past the ethernet header; cb = payload length;
    // uiType = network-order ethertype; uiFlags has PKTF_RECV_BROADCAST(0x1).
    // Rebuild a full frame (synthetic enet header + payload) and reuse the normal
    // receive path so IP/ARP dispatch, multicast handling, etc. all apply.
    Packet* p;
    EnetFrameHeader* hdr;

    UNREFERENCED_PARAMETER(self);

    // Runs at DISPATCH from xbdm's NIC receive DPC -- no DbgPrint (re-entrancy).
    if (!g_XbdmIfp)
        return;

    p = XnetAllocPacket(ENETHDRLEN + cb, PKTFLAG_NETPOOL);
    if (!p)
        return;   // out of memory: drop the frame

    p->pktflags    = 0;
    p->iphdrOffset = 0;
    p->data        = p->buf;
    p->datalen     = ENETHDRLEN + cb;
    p->recvifp     = (IfInfo*) g_XbdmIfp;

    hdr = (EnetFrameHeader*) p->data;
    if (uiFlags & 0x0001 /*PKTF_RECV_BROADCAST*/)
        CopyMem(hdr->dstaddr, EnetBroadcastAddr, ENETADDRLEN);
    else
        CopyMem(hdr->dstaddr, g_XbdmIfp->hwaddr, ENETADDRLEN);
    ZeroMem(hdr->srcaddr, ENETADDRLEN);     // unknown; keeps ARP conflict check quiet
    hdr->etherType = (WORD) uiType;         // already network order

    CopyMem(p->data + ENETHDRLEN, pv, cb);

    EnetReceiveFrame(g_XbdmIfp, p);
}

static void __thiscall RxdkCli_EnetPush(void* self) {
    UNREFERENCED_PARAMETER(self);
    // The server's transmit queue drained; resume our queued output.
    if (g_XbdmIfp)
        EnetStartOutput(g_XbdmIfp);
}

static DWORD __thiscall RxdkCli_GetXnAddr(void* self, RxdkXnAddrXbdm* px) {
    UNREFERENCED_PARAMETER(self);
    // Minimal: report our MAC, no IP yet. xbdm uses this for address reporting;
    // it is not on the connectivity path. (Full impl would return the live IP.)
    if (px) {
        if (g_XbdmIfp)
            CopyMem(px->abEnet, g_XbdmIfp->hwaddr, ENETADDRLEN);
        px->ina = 0;
    }
    return XNET_GET_XNADDR_NONE;   // 0x01
}

static void* g_XbdmClientVtbl[4] = {
    (void*) RxdkCli_XmitComplete,
    (void*) RxdkCli_EnetRecv,
    (void*) RxdkCli_EnetPush,
    (void*) RxdkCli_GetXnAddr,
};

static RxdkXbdmClient g_XbdmClient = { g_XbdmClientVtbl };

// --- Bridge control ---------------------------------------------------------

PRIVATE BOOL XbdmClientActive(void) {
    return g_pXbdmServer != NULL;
}

PRIVATE void XbdmClientXmit(Packet* pkt) {
    // pkt->data points at the ethernet header; datalen = full frame length.
    // NOTE: runs at DISPATCH from the TCP fast-timer DPC -- do NOT DbgPrint here
    // (DbgPrint goes over the same xbdm enet the bridge uses -> re-entrancy/hang).
    void** vt = RXDK_SRV_VTBL(g_pXbdmServer);
    ((RxdkFnXmit) vt[5])(g_pXbdmServer, pkt, pkt->data, pkt->datalen);
}

PRIVATE BOOL XbdmClientXmitReady(void) {
    void** vt = RXDK_SRV_VTBL(g_pXbdmServer);
    return ((RxdkFnXmitReady) vt[6])(g_pXbdmServer);
}

PRIVATE void XbdmClientStop(void) {
    if (g_pXbdmServer) {
        void** vt = RXDK_SRV_VTBL(g_pXbdmServer);
        KIRQL irql = RaiseToDpc();   // CXbdmServer::DetachClient runs at DISPATCH
        ((RxdkFnDetachClient) vt[3])(g_pXbdmServer);
        LowerFromDpc(irql);
        g_pXbdmServer = NULL;
        g_XbdmIfp = NULL;
    }
}

PRIVATE BOOL XbdmClientStart(IfEnet* ifp) {
    void* srv = KeGetCurrentPrcb()->DmEnetFunc;
    void** vt;
    NTSTATUS status;
    BYTE ea[ENETADDRLEN] = {0};
    BOOL fLinkUp = FALSE;

    if (!srv) {
        DbgPrint("xnet-init:   DmEnetFunc=NULL (retail/xemu): owning the NIC\n");
        return FALSE;   // no debug monitor: bring up the NIC ourselves
    }

    if (RXDK_SRV_COOKIE(srv) != XBDM_SERVER_COOKIE) {
        DbgPrint("xnet-init:   xbdm enet server cookie 0x%08x != 'XBD6'; owning the NIC\n",
                 RXDK_SRV_COOKIE(srv));
        return FALSE;
    }

    vt = RXDK_SRV_VTBL(srv);
    DbgPrint("xnet-init:   xbdm CXbdmServer=%p (cookie ok); InitClient...\n", srv);

    status = ((RxdkFnInitClient) vt[1])(srv, cfgRecvQLength, cfgXmitQLength, ea, &fLinkUp);
    if (!NT_SUCCESS(status)) {
        DbgPrint("xnet-init:   CXbdmServer::InitClient failed 0x%08x; owning the NIC\n", status);
        return FALSE;
    }

    CopyMem(ifp->hwaddr, ea, ENETADDRLEN);
    ifp->hwaddrlen = ENETADDRLEN;
    g_XbdmIfp = ifp;
    g_pXbdmServer = srv;

    ((RxdkFnAttachClient) vt[2])(srv, &g_XbdmClient);

    DbgPrint("xnet-init:   xbdm client attached; mac=%02x:%02x:%02x:%02x:%02x:%02x link=%d\n",
             ea[0], ea[1], ea[2], ea[3], ea[4], ea[5], fLinkUp);

    if (fLinkUp)
        ifp->flags |= IFFLAG_CONNECTED_BOOT;

    return TRUE;
}


NTSTATUS
EnetInitialize(
    IfInfo** newifp
    )

/*++

Routine Description:

    Initialize the Ethernet interface.

Arguments:

    NONE

Return Value:

    Status code

--*/

{
    IfEnet* ifp;
    NTSTATUS status;

    DbgPrint("xnet-init:   EnetInitialize enter (irql=%d)\n", (int)KeGetCurrentIrql());

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    #ifdef DVTSNOOPBUG
    status = XnetUncachedPoolInit();
    if (!NT_SUCCESS(status)) return NETERR_MEMORY;
    #endif

    // Allocate memory to hold our interface structure

    ifp = SysAlloc0(sizeof(IfEnet), PTAG_ENET);
    if (!ifp) return NETERR_MEMORY;
    ifp->refcount = 1;
    ifp->ifname = "Ethernet";
    ifp->magicCookie = 'TENE';

    // Interface functions
    ifp->Delete = (IfDeleteProc) EnetDelete;
    ifp->StartOutput = (IfStartOutputProc) EnetStartOutput;
    ifp->Timer = (IfTimerProc) EnetTimer;
    ifp->Ioctl = (IfIoctlProc) EnetIoctl;

    ifp->iftype = IFTYPE_ETHERNET;
    ifp->framehdrlen = ENETHDRLEN;
    ifp->mtu = ENET_MAXDATASIZE;

    // Dev kit: if the debug monitor owns the NIC, attach to it as a client and
    // skip our own hardware bring-up (xbdm drives the NIC; we share it).
    if (XbdmClientStart(ifp)) {
        ifp->flags |= IFFLAG_UP |
                      IFFLAG_DHCP_ENABLED |
                      IFFLAG_BCAST |
                      IFFLAG_MCAST;
        *newifp = (IfInfo*) ifp;
        IfSetIpAddr(*newifp, 0, 0);
        DbgPrint("xnet-init:   enet ready via xbdm debug-monitor share\n");
        return NETERR_OK;
    }

    DbgPrint("xnet-init:   EnetInitialize: NicInitialize...\n");
    // Initialize the NIC interface
    status = NicInitialize(ifp);
    if (!NT_SUCCESS(status)) goto failed;

    // Now mark the interface as fully initialized
    // and start handling interrupts.

    ifp->flags |= IFFLAG_UP |
                  IFFLAG_DHCP_ENABLED |
                  IFFLAG_BCAST |
                  IFFLAG_MCAST;

    *newifp = (IfInfo*) ifp;
    IfSetIpAddr(*newifp, 0, 0);

    // Set the enet interface information in the process control block
    SET_DBGMON_ENETINFO(ifp);

    // Register shutdown notification routine
    EnetShutdownNotification.NotificationRoutine = EnetShutdownNotifyProc;
    HalRegisterShutdownNotification(&EnetShutdownNotification, TRUE);
    EnetShutdownNotifyRegistered = TRUE;

    NicEnableInterrupt();
    return NETERR_OK;

failed:
    WARNING_("EnetInitialize failed: 0x%x", status);
    EnetDelete(ifp);
    return status;
}

