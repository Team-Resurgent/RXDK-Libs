#ifndef RXDK_XAPI_KPRCB_BRIDGE_H
#define RXDK_XAPI_KPRCB_BRIDGE_H

/*
 * Minimal KPRCB / KeGetCurrentPrcb for vendor dm.h (DebugMonitorData @ 0x250).
 * Public xboxkrnl headers omit this internal layout.
 */

#include <xapi/nt_bridge.h>

typedef struct _KPRCB {
    UCHAR Padding[0x250];
    PVOID DebugMonitorData;
} KPRCB, *PKPRCB;

typedef struct _KPCR {
    NT_TIB NtTib;
    struct _KPCR *SelfPcr;
    PKPRCB Prcb;
    KIRQL Irql;
    KPRCB PrcbData;
} KPCR, *PKPCR;

static __inline PKPRCB KeGetCurrentPrcb(void)
{
    PKPCR pcr;
    __asm__ volatile ("movl %%fs:0, %0" : "=r"(pcr));
    return &pcr->PrcbData;
}

#endif
