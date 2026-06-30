#pragma once
#define RXDK_XAPIP_H


/*
 * Internal libxapi precompile header (replaces leak private/ntos/xapi/inc/xapip.h).
 * Includes RXDK xapi.h (xboxkrnl) instead of vendor ntos.h / sdk/nt.h.
 */

#ifndef _XAPIP_
#define _XAPIP_

#include "compile.h"
#include <ntrtl.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <ntdddisk.h>

#ifdef __cplusplus
}
#endif

#include "xtl.h"
#include <xdbg.h>
#include <xapidrv.h>
#include <ldr.h>
#include <xbeimage.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const IMAGE_TLS_DIRECTORY _tls_used;
extern ULONG _tls_index;

extern HANDLE XapiProcessHeap;

extern RTL_CRITICAL_SECTION XapiProcessLock;

#define XapiAcquireProcessLock() RtlEnterCriticalSection(&XapiProcessLock)
#define XapiReleaseProcessLock() RtlLeaveCriticalSection(&XapiProcessLock)

typedef struct _X_CACHE_DB_ENTRY {
    DWORD dwTitleId;
    ULONG nCacheIndex;
    BOOL fUsed;
} X_CACHE_DB_ENTRY, *PX_CACHE_DB_ENTRY;

static __inline BOOL XapiIsXapiThread(void)
{
    return (KeGetCurrentIrql() < DISPATCH_LEVEL &&
            KeGetCurrentThread()->TlsData != NULL);
}

VOID XapiInitProcess(VOID);
VOID XapiBootToDash(DWORD dwReason, DWORD dwParameter1, DWORD dwParameter2);

BOOL __attribute__((__stdcall__)) XapiFormatFATVolume(IN POBJECT_STRING pcVolume);
BOOL __attribute__((__stdcall__)) XapiFormatFATVolumeEx(IN POBJECT_STRING pcVolume, IN ULONG BytesPerCluster);

NTSTATUS XapiGetCachePartitions(
    IN PX_CACHE_DB_ENTRY pCacheEntriesBuffer,
    IN UINT cbBufferSize,
    OUT PDWORD pdwNumCacheEntries);

VOID XapiDeleteCachePartition(IN DWORD dwTitleId);

NTSTATUS XapiValidateDiskPartition(POBJECT_STRING PartitionName);
NTSTATUS XapiValidateDiskPartitionEx(POBJECT_STRING PartitionName, ULONG BytesPerCluster);

NTSTATUS XapiMapLetterToDirectory(
    PCOBJECT_STRING pcDriveString,
    PCOBJECT_STRING pcPathString,
    PCOSTR pcszTitleId,
    BOOL fCreateDirectory,
    LPCWSTR pcszTitleName,
    BOOL fUpdateTimestamp);

BOOL XapiDeleteValueInMetaFile(HANDLE hMetaFile, LPCWSTR pszTag);

void XapiInitAutoPowerDown(void);

#define CONSTANT_OBJECT_STRING(s) \
    { sizeof(s) - sizeof(OCHAR), sizeof(s), s }

#define DwordToStringO(dword, dwordstr) \
    { soprintf(dwordstr, OTEXT("%08lx"), dword); }

#define CCHMAX_HEX_DWORD 9

// STDCALL: defined in the stdcall-default USB driver, called from the cdecl core.
NTSTATUS STDCALL MU_CreateDeviceObject(IN ULONG Port, IN ULONG Slot, IN POBJECT_STRING DeviceName);
VOID STDCALL MU_CloseDeviceObject(IN ULONG Port, IN ULONG Slot);
PDEVICE_OBJECT STDCALL MU_GetExistingDeviceObject(IN ULONG Port, IN ULONG Slot);

#ifdef DBG
extern ULONG MU_MaxUserDevices;
#endif

#ifdef __cplusplus
}
#endif

#endif /* _XAPIP_ */

