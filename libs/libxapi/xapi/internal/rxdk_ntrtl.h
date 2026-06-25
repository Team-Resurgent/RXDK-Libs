#ifndef RXDK_XAPI_NTRTL_H
#define RXDK_XAPI_NTRTL_H

/*
 * Shadow of leak ntrtl.h — memory Rtl macros for libxapi, not full sdk/ntrtl.h.
 * Heap/random implement Rtl* locally; no xboxkrnl/api/rtl.h import surface here.
 */

#ifndef _NTRTL_
#define _NTRTL_

#ifndef NTSYSAPI
#define NTSYSAPI
#endif

#include <stddef.h>
#include <string.h>
#include <xboxkrnl/xboxdef.h>

#ifndef RtlZeroMemory
#define RtlZeroMemory(Destination, Length) memset((Destination), 0, (Length))
#endif
#ifndef RtlFillMemory
#define RtlFillMemory(Destination, Length, Fill) memset((Destination), (Fill), (Length))
#endif
#ifndef RtlCopyMemory
#define RtlCopyMemory(Destination, Source, Length) memcpy((Destination), (Source), (Length))
#endif
#ifndef RtlMoveMemory
#define RtlMoveMemory(Destination, Source, Length) memmove((Destination), (Source), (Length))
#endif

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field) ((LONG)(LONG_PTR)&(((type *)0)->field))
#endif

#define INITIALIZED_LIST_ENTRY(ListEntry) \
    LIST_ENTRY ListEntry = { &ListEntry, &ListEntry }

#define INITIALIZED_CRITICAL_SECTION(CriticalSection) \
    RTL_CRITICAL_SECTION CriticalSection = { \
        1, \
        FALSE, \
        (UCHAR)(FIELD_OFFSET(RTL_CRITICAL_SECTION, LockCount) / sizeof(LONG)), \
        FALSE, \
        FALSE, \
        &CriticalSection.Synchronization.Event.WaitListHead, \
        &CriticalSection.Synchronization.Event.WaitListHead, \
        -1, \
        0, \
        NULL \
    }

#endif /* _NTRTL_ */
#endif /* RXDK_XAPI_NTRTL_H */
