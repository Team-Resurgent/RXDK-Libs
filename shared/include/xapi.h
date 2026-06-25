#pragma once
#define RXDK_XAPI_H


/*
 * Public libxapi title surface (Xbox-only).
 *
 * Kernel types/APIs from xboxkrnl; Win32 shims from xapi/win32; Xbox/XInput from xbox.h + xkbd.h.
 * Internal compile headers (precompile.h, site/profile.h) are not exposed.
 */

#ifndef _XBOX
#define _XBOX 1
#endif
#define XBOX 1

#ifndef _X86_
#define _X86_ 1
#endif
#ifndef i386
#define i386 1
#endif
#ifndef _M_IX86
#define _M_IX86 600
#endif
#ifndef _WIN32
#define _WIN32 1
#endif

#ifndef _XTL_
#define _XTL_
#endif
#ifndef _NTURTL_
#define _NTURTL_
#endif
#ifndef _INC_XTL
#define _INC_XTL
#endif
#ifndef _INC_WINDOWS
#define _INC_WINDOWS
#endif

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <xboxkrnl/xboxkrnl.h>

#ifndef NT_INCLUDED
#define NT_INCLUDED
#endif

#ifndef NtGetTickCount
#define NtGetTickCount() ((DWORD)KeTickCount)
#endif

/* PE32 / TLS layout (xapi_boot.c, title TLS image). */
typedef struct _IMAGE_THUNK_DATA32 {
    union {
        DWORD ForwarderString;
        DWORD Function;
        DWORD Ordinal;
        DWORD AddressOfData;
    } u1;
} IMAGE_THUNK_DATA32, *PIMAGE_THUNK_DATA32;

typedef IMAGE_THUNK_DATA32 IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA32 PIMAGE_THUNK_DATA;

typedef struct _IMAGE_TLS_DIRECTORY32 {
    DWORD StartAddressOfRawData;
    DWORD EndAddressOfRawData;
    DWORD AddressOfIndex;
    DWORD AddressOfCallBacks;
    DWORD SizeOfZeroFill;
    DWORD Characteristics;
} IMAGE_TLS_DIRECTORY32, *PIMAGE_TLS_DIRECTORY32;

typedef IMAGE_TLS_DIRECTORY32 IMAGE_TLS_DIRECTORY;
typedef PIMAGE_TLS_DIRECTORY32 PIMAGE_TLS_DIRECTORY;

#define RXDK_TLS_IMAGE_OFF_CURRENT_FIBER 4
#define RXDK_TLS_IMAGE_OFF_THREAD_FIBER_DATA 8
#define RXDK_TLS_IMAGE_OFF_LAST_ERROR 24
#define RXDK_TLS_IMAGE_OFF_TLS_SLOTS 28
#define RXDK_TLS_IMAGE_SIZE 284

struct _RTL_HEAP_PARAMETERS;

PVOID __stdcall RtlCreateHeap(
    ULONG Flags,
    PVOID HeapBase,
    SIZE_T ReserveSize,
    SIZE_T CommitSize,
    PVOID Lock,
    struct _RTL_HEAP_PARAMETERS *Parameters);
PVOID __stdcall RtlDestroyHeap(PVOID HeapHandle);
PVOID __stdcall RtlAllocateHeap(PVOID HeapHandle, ULONG Flags, SIZE_T Size);
PVOID __stdcall RtlReAllocateHeap(PVOID HeapHandle, ULONG Flags, PVOID BaseAddress, SIZE_T Size);
BOOLEAN __stdcall RtlFreeHeap(PVOID HeapHandle, ULONG Flags, PVOID BaseAddress);
SIZE_T __stdcall RtlSizeHeap(PVOID HeapHandle, ULONG Flags, PVOID BaseAddress);

#include <stdarg.h>

#ifndef _NTDEF_
#include "win32_bridge.h"
#else
#include "winnt_skipped.h"
#endif

#include <windef.h>
#include <winbase.h>
#include <xbox.h>
#include <xkbd.h>

#ifdef __cplusplus
}
#endif

