/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dbgmon.h

Abstract:

    Macros to hide the difference between normal build
    and the special build for the debug monitor

Revision History:

    07/21/2000 davidx
        Created it.

Note:

    This header file must not be included into multiple C files!

--*/

#ifndef _DBGMON_H
#define _DBGMON_H

// IP packet reception function.
// RXDK: these function pointers cross the boundary with the debug monitor
// (xbdm), which is built __stdcall. clang defaults to cdecl -> calling
// DmEnetFunc (and the DM calling our IpReceiveProc/FreePktProc back) corrupts
// the stack. Pin every DM-boundary pointer __stdcall.
typedef VOID (__stdcall *IPRECEIVEPROC)(Packet*);
typedef VOID (__stdcall *FREEPKTPROC)(Packet*);
typedef VOID* (__stdcall *UNCACHEDALLOCPROC)(SIZE_T, ULONG);
typedef VOID (__stdcall *UNCACHEDFREEPROC)(VOID*);

typedef struct _ENETINITPARAMS {
    IN IPRECEIVEPROC IpReceiveProc;
    IN FREEPKTPROC FreePktProc;
    OUT UNCACHEDALLOCPROC UncachedAllocProc;
    OUT UNCACHEDFREEPROC UncachedFreeProc;
} ENETINITPARAMS;

// Enet initialization function provided by the debug monitor
typedef IfEnet* (__stdcall *DBGMON_ENETINITPROC)(ENETINITPARAMS*);

#ifndef BUILD_FOR_DEBUGGER

// Pass the received frame up to the IP layer
#define IPRECEIVEPACKET IpReceivePacket

// Dispose of a packet after transmission
#define COMPLETEPACKET XnetCompletePacket

// Check to see if we should delete the enet interface
#define IFENET_DELETE_CHECK(ifp) \
        ASSERT((ifp)->refcount == 1); \
        (ifp)->refcount--

// Set the enet interface information in the process control block
#define SET_DBGMON_ENETINFO(ifp)

//
// Function called by the debug monitor enet driver to
// dispose of packets that the regular stack has passed down
//
INLINE VOID __stdcall FreePacketCallback(Packet* pkt) {  /* RXDK: DM calls this -> __stdcall */
    static int _fpd = 0; ++_fpd; if (_fpd <= 4 || (_fpd % 64) == 0) DbgPrint("xnet-init:    FreePacketCallback call #%d\n", _fpd);
    XnetFreePacket(pkt);
}

//
// Check if the debug monitor has already initialized the enet interface
//
#define CHECK_DBGMON_ENETINFO() { \
            DBGMON_ENETINITPROC enetInitProc; \
            enetInitProc = (DBGMON_ENETINITPROC) KeGetCurrentPrcb()->DmEnetFunc; \
            DbgPrint("xnet-init:   Prcb=%p DmEnetFunc=%p\n", (void*)KeGetCurrentPrcb(), (void*)enetInitProc); \
            if (enetInitProc) { \
                ENETINITPARAMS initParams = { IpReceivePacket, FreePacketCallback, }; \
                DbgPrint("xnet-init:   calling DmEnetFunc(&ip=%p,free=%p)...\n", (void*)IpReceivePacket, (void*)FreePacketCallback); \
                ifp = enetInitProc(&initParams); \
                DbgPrint("xnet-init:   DmEnetFunc returned ifp=%p (alloc=%p free=%p)\n", (void*)ifp, (void*)initParams.UncachedAllocProc, (void*)initParams.UncachedFreeProc); \
                *newifp = (IfInfo*) ifp; \
                XnetUncachedAllocProc = initParams.UncachedAllocProc; \
                XnetUncachedFreeProc = initParams.UncachedFreeProc; \
                IfSetIpAddr(*newifp, ifp->ipaddr, ifp->addrmask); \
                DhcpSetDefaultGateways(*newifp); \
                DbgPrint("xnet-init:   dbgmon enet ready\n"); \
                return NETERR_OK; \
            } \
        }

#else // BUILD_FOR_DEBUGGER

#include "dm.h"
#include "xbdm.h"

// Pass the received frame up to the IP layer
VOID IPRECEIVEPACKET(Packet* pkt);

// Dispose of a packet after transmission
VOID COMPLETEPACKET(Packet* pkt, NTSTATUS status);

// Check to see if we should delete the enet interface
VOID IFENET_DELETE_CHECK(IfEnet* ifp);

// Set the enet interface information in the process control block
VOID SET_DBGMON_ENETINFO(IfEnet* ifp);

// Check if the debug monitor has already initialized the enet interface
#define CHECK_DBGMON_ENETINFO()

#endif // BUILD_FOR_DEBUGGER

#endif // !_DBGMON_H

