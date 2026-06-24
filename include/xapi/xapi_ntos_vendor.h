#ifndef RXDK_XAPI_NTOS_VENDOR_H
#define RXDK_XAPI_NTOS_VENDOR_H

/*
 * Leak NTOS internal headers that libxapi still references for layout/IOCTL
 * definitions. Included only after xapi.h (xboxkrnl types). Does not include
 * sdk/nt.h.
 */

#pragma warning(disable : 4214 4201 4115 4200 4100 4514 4057 4127)

#include "ntosdef.h"
#include "exboosts.h"
#include "bugcodes.h"
#include "init.h"

#ifdef _X86_
#include "i386.h"
#endif

#include "ke.h"
#include "kd.h"
#include "ex.h"
#include "ps.h"
#include "io.h"
#include "ob.h"
#include "mm.h"
#include "fscache.h"
#include "xpcicfg.h"

#ifndef RXDK_TIME_FIELDS
#define RXDK_TIME_FIELDS
typedef struct _TIME_FIELDS {
    CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT Milliseconds;
    CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;
#endif

#include "hal.h"

#define _NTDDK_

#if !defined(_NTSYSTEM_)
extern POBJECT_TYPE ExEventObjectType;
extern POBJECT_TYPE ExMutantObjectType;
extern POBJECT_TYPE ExSemaphoreObjectType;
extern POBJECT_TYPE ExTimerObjectType;
extern POBJECT_TYPE PsProcessObjectType;
extern POBJECT_TYPE PsThreadObjectType;
extern POBJECT_TYPE IoCompletionObjectType;
extern POBJECT_TYPE IoDeviceObjectType;
extern POBJECT_TYPE IoFileObjectType;
extern POBJECT_TYPE ObDirectoryObjectType;
extern POBJECT_TYPE ObSymbolicLinkObjectType;
#endif

#endif /* RXDK_XAPI_NTOS_VENDOR_H */
