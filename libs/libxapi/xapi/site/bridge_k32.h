#ifndef RXDK_K32_BRIDGE_H
#define RXDK_K32_BRIDGE_H

#include <xboxkrnl/xboxdef.h>
#include <xboxkrnl/types/common.h>

/* Force-included for libxapi k32 slice (SEH + paging + IOCTL helpers). */
#include "tls_layout.h"

#ifdef try
#undef try
#endif
#ifdef except
#undef except
#endif
#ifdef leave
#undef leave
#endif
#ifdef finally
#undef finally
#endif
/* Vendor xAPI sources use MSVC SEH (try/leave/finally). leave must jump past the
 * error epilogue; an empty leave lets success paths fall through to failure. */
#define try if (1)
#define except(x) else if (0)
#define leave goto __rxdk_seh_finally
#define finally __rxdk_seh_finally: if (1)
#ifndef GetExceptionCode
#define GetExceptionCode() ((DWORD)0)
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#ifndef PAGE_READWRITE
#define PAGE_READWRITE 0x04
#define PAGE_READONLY 0x02
#define PAGE_WRITECOMBINE 0x400
#define PAGE_NOCACHE 0x200
#endif

#ifndef MAXULONG_PTR
#define MAXULONG_PTR (~((ULONG_PTR)0))
#endif
#ifndef BYTE_OFFSET
#define BYTE_OFFSET(Va) ((ULONG)((LONG_PTR)(Va) & (PAGE_SIZE - 1)))
#endif

#ifndef FILE_READ_ACCESS
#define FILE_READ_ACCESS 0x0001
#endif

#ifndef FSCTL_READ_VOLUME_METADATA
#define FSCTL_READ_VOLUME_METADATA CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 71, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

#ifndef FAT_VOLUME_NAME_LENGTH
#define FAT_VOLUME_NAME_LENGTH 32
#endif

#ifndef FSCTL_VOLUME_METADATA
typedef struct _FSCTL_VOLUME_METADATA {
    DWORD ByteOffset;
    DWORD TransferLength;
    PVOID TransferBuffer;
} FSCTL_VOLUME_METADATA, *PFSCTL_VOLUME_METADATA;
#endif

#ifndef STATUS_CANCELLED
#define STATUS_CANCELLED ((NTSTATUS)0xC0000120L)
#endif
#ifndef IO_NO_INCREMENT
#define IO_NO_INCREMENT 0
#endif

#ifndef HEAP_REALLOC_IN_PLACE_ONLY
#define HEAP_REALLOC_IN_PLACE_ONLY 0x00000010
#endif

#ifndef LMEM_ZEROINIT
#define LMEM_ZEROINIT 0x0040
#define LMEM_MOVEABLE 0x0002
#endif

#ifndef TIMERR_NOERROR
#define TIMERR_NOERROR 0
#endif

#ifndef TIME_MS
#define TIME_MS 0x0001
#endif

#ifndef LPMMTIME
typedef struct mmtime_tag {
    UINT wType;
    union {
        DWORD ms;
        DWORD sample;
        DWORD cb;
        DWORD ticks;
    } u;
} MMTIME, *PMMTIME, *LPMMTIME;
#endif

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(ArgumentPointer) \
    ((ULONG_PTR)(ArgumentPointer) != (ULONG_PTR)(NULL))
#endif

#ifndef Int32x32To64
#define Int32x32To64(a, b) ((LONGLONG)((LONGLONG)(LONG)(a) * (LONG)(b)))
#endif
#ifndef UInt32x32To64
#define UInt32x32To64(a, b) ((ULONGLONG)((ULONGLONG)(DWORD)(a) * (DWORD)(b)))
#endif

#ifndef RtlInitObjectString
#define RtlInitObjectString RtlInitAnsiString
#endif

#ifndef NtGetTickCount
#define NtGetTickCount() ((DWORD)KeTickCount)
#endif

#ifndef KeLowerIrql
#define KeLowerIrql(a) KfLowerIrql(a)
#endif

#ifndef KeInitializeTimer
#define KeInitializeTimer(Timer) KeInitializeTimerEx((Timer), NotificationTimer)
#endif

#ifndef ObDereferenceObject
#define ObDereferenceObject(a) ObfDereferenceObject(a)
#endif

#ifndef HandleToUlong
#define HandleToUlong(h) ((ULONG)(ULONG_PTR)(h))
#endif

#ifndef KeReadStateThread
#define KeReadStateThread(Thread) ((BOOLEAN)(Thread)->Header.SignalState)
#endif

#ifndef PsGetCurrentThread
#define PsGetCurrentThread() ((PETHREAD)KeGetCurrentThread())
#endif

#ifndef PsGetCurrentThreadId
#define PsGetCurrentThreadId() (PsGetCurrentThread()->UniqueThread)
#endif

#ifndef ocscpy
#define ocscpy strcpy
#endif
#ifndef ocslen
#define ocslen strlen
#endif
#ifndef _snoprintf
#define _snoprintf _snprintf
#endif
#ifndef lstrcpynO
#define lstrcpynO lstrcpynA
#endif

#ifndef RXDK_CLANG_LASTERROR
#define RXDK_CLANG_LASTERROR 1
#endif

#include "heap_rtl.h"

#endif /* RXDK_K32_BRIDGE_H */
