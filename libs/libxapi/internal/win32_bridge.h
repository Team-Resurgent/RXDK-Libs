#pragma once
#define RXDK_XAPI_XDK_BRIDGE_H


/*
 * Win32 typedefs windef expects from winnt.h when NT_INCLUDED skips it (leak xAPI
 * pattern: nt.h / xboxkrnl first, then xtl).
 */

#include <xboxkrnl/xboxdef.h>
#include <xboxkrnl/types/common.h>

#ifndef UINT_PTR
typedef unsigned int UINT_PTR, *PUINT_PTR;
typedef int INT_PTR, *PINT_PTR;
typedef ULONG_PTR *PULONG_PTR;
typedef LONG_PTR *PLONG_PTR;
#endif

#ifndef READ_CONTROL
#define DELETE                           (0x00010000L)
#define READ_CONTROL                     (0x00020000L)
#define WRITE_DAC                        (0x00040000L)
#define WRITE_OWNER                      (0x00080000L)
#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)
#define STANDARD_RIGHTS_READ             (READ_CONTROL)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)
#define STANDARD_RIGHTS_ALL              (0x001F0000L)
#endif

#ifndef SYNCHRONIZE
#define SYNCHRONIZE                      (0x00100000L)
#endif

#ifndef IO_COMPLETION_ALL_ACCESS
#define IO_COMPLETION_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3)
#endif

#ifndef FILE_WRITE_ATTRIBUTES
#define FILE_READ_DATA                   0x0001
#define FILE_WRITE_DATA                  0x0002
#define FILE_READ_ATTRIBUTES             0x0080
#define FILE_WRITE_ATTRIBUTES            0x0100
#define FILE_GENERIC_WRITE               (STANDARD_RIGHTS_WRITE | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA | SYNCHRONIZE)
#define FILE_WRITE_EA                    0x0010
#define FILE_APPEND_DATA                 0x0004
#endif

#ifndef FILE_OPEN
#define FILE_OPEN                        0x00000001
#define FILE_OPEN_FOR_BACKUP_INTENT      0x00004000
#define FILE_ATTRIBUTE_VALID_SET_FLAGS   0x000031a7
#endif

#ifndef FILE_DEVICE_CONTROLLER
#define FILE_DEVICE_CONTROLLER           0x00000004
#define FILE_DEVICE_DISK                 0x00000007
#define FILE_DEVICE_FILE_SYSTEM          0x00000009
#define METHOD_BUFFERED                  0
#define FILE_READ_ACCESS                 0x0001
#define FILE_WRITE_ACCESS                0x0002
#define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS                  0
#endif

#ifndef FSCTL_DISMOUNT_VOLUME
#define FSCTL_DISMOUNT_VOLUME            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef IRP_MJ_READ
#define IRP_MJ_READ                      0x03
#endif

#ifndef FILE_SYNCHRONOUS_IO_ALERT
#define FILE_SYNCHRONOUS_IO_ALERT        0x00000010
#endif

#ifndef MEM_COMMIT
#define MEM_COMMIT                       0x1000
#define MEM_RESERVE                      0x2000
#define MEM_RELEASE                      0x8000
#endif

#ifndef ANSI_NULL
#define ANSI_NULL                        0
#endif

#ifndef MM_HIGHEST_USER_ADDRESS
#define MM_HIGHEST_USER_ADDRESS          ((PVOID)(ULONG_PTR)0x7FFEFFFF)
#endif
#ifndef MM_LOWEST_USER_ADDRESS
#define MM_LOWEST_USER_ADDRESS           ((PVOID)(ULONG_PTR)0x00010000)
#endif

#ifndef TLS_MINIMUM_AVAILABLE
#define TLS_MINIMUM_AVAILABLE            64
#endif

#ifndef EVENT_INCREMENT
#define EVENT_INCREMENT                  1
#endif

#ifndef HEAP_GENERATE_EXCEPTIONS
#define HEAP_GENERATE_EXCEPTIONS         0x00000004
#define HEAP_ZERO_MEMORY                 0x00000008
#endif

#ifndef ULONGLONG
typedef unsigned long long ULONGLONG, *PULONGLONG;
#endif

#ifndef TIME_PERIODIC
#define TIME_PERIODIC                    0x0001
#define TIME_CALLBACK_FUNCTION           0x0000
#define TIME_CALLBACK_EVENT_SET          0x0010
#define TIME_CALLBACK_EVENT_PULSE        0x0020
#endif

#ifndef TIMERR_NOCANDO
#define TIMERR_BASE                      96
#define TIMERR_NOCANDO                   (TIMERR_BASE + 1)
#define MMSYSERR_NOERROR                 0
#endif

#ifndef CALLBACK
#define CALLBACK __stdcall
#endif

#ifndef MMRESULT
typedef UINT MMRESULT;
#endif

#ifndef LPTIMECALLBACK
typedef void (CALLBACK TIMECALLBACK)(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
typedef TIMECALLBACK *LPTIMECALLBACK;
#endif

#ifndef STATUS_WAIT_1
#define STATUS_WAIT_1                    ((NTSTATUS)0x00000001L)
#endif

#ifndef MAXLONG
#define MAXLONG 0x7fffffff
#endif

#ifndef EXCEPTION_CONTINUE_EXECUTION
#define EXCEPTION_CONTINUE_EXECUTION    (-1)
#define EXCEPTION_CONTINUE_SEARCH       0
#define EXCEPTION_EXECUTE_HANDLER       1
#endif

#ifndef THREAD_BASE_PRIORITY_LOWRT
#define THREAD_BASE_PRIORITY_LOWRT  15
#define THREAD_BASE_PRIORITY_MAX    2
#define THREAD_BASE_PRIORITY_MIN    (-2)
#define THREAD_BASE_PRIORITY_IDLE   (-15)
#endif

#ifndef CLOCK_QUANTUM_DECREMENT
#define CLOCK_QUANTUM_DECREMENT 3
#endif

typedef NTSTATUS (__stdcall *PRTL_HEAP_COMMIT_ROUTINE)(
    IN PVOID Base,
    IN OUT PVOID *CommitAddress,
    IN OUT PSIZE_T CommitSize
    );

#ifndef RTL_HEAP_PARAMETERS
typedef struct _RTL_HEAP_PARAMETERS {
    ULONG Length;
    SIZE_T SegmentReserve;
    SIZE_T SegmentCommit;
    SIZE_T DeCommitFreeBlockThreshold;
    SIZE_T DeCommitTotalFreeThreshold;
    SIZE_T MaximumAllocationSize;
    SIZE_T VirtualMemoryThreshold;
    SIZE_T InitialCommit;
    SIZE_T InitialReserve;
    PRTL_HEAP_COMMIT_ROUTINE CommitRoutine;
    SIZE_T Reserved[2];
} RTL_HEAP_PARAMETERS, *PRTL_HEAP_PARAMETERS;
#endif

#ifndef HEAP_GROWABLE
#define HEAP_NO_SERIALIZE               0x00000001
#define HEAP_GROWABLE                   0x00000002
#define HEAP_CLASS_0                    0x00000000
#define HEAP_CLASS_1                    0x00001000
#define HEAP_CLASS_MASK                 0x0000F000
#endif

#ifndef IN
#define IN
#define OUT
#define OPTIONAL
#endif

#ifndef DECLARE_HANDLE
#define DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#endif

#ifndef FORCEINLINE
#define FORCEINLINE static __inline
#endif

#ifndef KERNEL_STACK_SIZE
#define KERNEL_STACK_SIZE 12288
#endif

typedef CHAR *NPSTR, *LPSTR, *PSTR;
typedef CONST CHAR *LPCSTR, *PCSTR, *LPCCH, *PCCH;
typedef WCHAR *NPWSTR, *LPWSTR, *PWSTR, *PWCHAR, *LPWCH;
typedef CONST WCHAR *LPCWSTR, *PCWSTR;
typedef LPSTR LPTCH, PTSTR, POSTR;
typedef LPWSTR PTWSTR;
typedef LPWSTR LPTSTR, PTCHAR;


#ifndef FAR
#define FAR
#endif
#ifndef NEAR
#define NEAR
#endif

#ifndef CONST
#define CONST const
#endif

typedef LONG HRESULT;

#ifndef GENERIC_READ
#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)
#endif

#ifndef SYNCHRONIZE
#define SYNCHRONIZE                      (0x00100000L)
#endif

#ifndef FILE_LIST_DIRECTORY
#define FILE_LIST_DIRECTORY              (0x0001)
#endif

#ifndef FILE_SHARE_READ
#define FILE_SHARE_READ                  0x00000001
#define FILE_SHARE_WRITE                 0x00000002
#define FILE_SHARE_DELETE                0x00000004
#endif

#ifndef CREATE_NEW
#define CREATE_NEW                       1
#define CREATE_ALWAYS                    2
#define OPEN_EXISTING                    3
#define OPEN_ALWAYS                      4
#define TRUNCATE_EXISTING                5
#endif

#ifndef FILE_SYNCHRONOUS_IO_NONALERT
#define FILE_SYNCHRONOUS_IO_NONALERT     0x00000020
#define FILE_DIRECTORY_FILE              0x00000001
#define FILE_OPEN_FOR_FREE_SPACE_QUERY   0x00800000
#endif

#ifndef FILE_OPEN_IF
#define FILE_OPEN_IF                     0x00000003
#define FILE_NON_DIRECTORY_FILE          0x00000040
#endif

#ifndef FILE_WRITE_THROUGH
#define FILE_WRITE_THROUGH               0x00000002
#endif

#ifndef FILE_CREATE
#define FILE_SUPERSEDE                   0x00000000
#define FILE_CREATE                      0x00000002
#define FILE_OVERWRITE                   0x00000004
#define FILE_OVERWRITE_IF                0x00000005
#define FILE_SEQUENTIAL_ONLY             0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING   0x00000008
#define FILE_RANDOM_ACCESS               0x00000800
#define FILE_DELETE_ON_CLOSE             0x00001000
#define FILE_ATTRIBUTE_VALID_FLAGS       0x00007fb7
#define FILE_OVERWRITTEN                 0x00000003
#define FILE_OPENED                      0x00000001
#endif

#ifndef PAGE_READWRITE
#define PAGE_READWRITE                   0x04
#endif

#ifndef BREAKPOINT_PRINT
#define BREAKPOINT_PRINT                 1
#endif

#ifndef DWORDLONG
typedef unsigned long long DWORDLONG, *PDWORDLONG;
#endif
#ifndef UNICODE_NULL
#define UNICODE_NULL                     ((WCHAR)0)
#endif

#ifndef LCID
typedef DWORD LCID;
#endif

#ifndef LANGID
typedef WORD LANGID;
#endif

#ifndef LPCTSTR
typedef LPCSTR LPCTSTR, PCTSTR;
#endif

#ifndef MAXIMUM_WAIT_OBJECTS
#define MAXIMUM_WAIT_OBJECTS 64
#endif

#ifndef TIME_ZONE_ID_UNKNOWN
#define TIME_ZONE_ID_UNKNOWN  0
#define TIME_ZONE_ID_STANDARD 1
#define TIME_ZONE_ID_DAYLIGHT 2
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#ifndef PAGE_SHIFT
#define PAGE_SHIFT 12
#endif

#ifndef POINTER_32
#define POINTER_32
#endif

#ifndef EXCEPTION_CHAIN_END
#define EXCEPTION_CHAIN_END              ((ULONG_PTR)-1)
#endif

#if !defined(__XBOXDEF_H__) && !defined(_NTDEF_)
typedef struct _CSTRING {
    USHORT Length;
    USHORT MaximumLength;
    CONST CHAR *Buffer;
} CSTRING, *PCSTRING;

typedef CSTRING COBJECT_STRING;
typedef PCSTRING PCOBJECT_STRING;
#endif

#ifndef VOID
#define VOID void
#endif

#ifndef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT __declspec(dllimport)
#endif
#ifndef DECLSPEC_NORETURN
#define DECLSPEC_NORETURN __declspec(noreturn)
#endif
#ifndef DECLSPEC_SELECTANY
#define DECLSPEC_SELECTANY __declspec(selectany)
#endif

#if !defined(MIDL_PASS)
typedef PCONTEXT LPCONTEXT;
typedef PEXCEPTION_RECORD LPEXCEPTION_RECORD;
#ifndef RXDK_EXCEPTION_POINTERS_DEFINED
#define RXDK_EXCEPTION_POINTERS_DEFINED
typedef struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;
#endif
typedef PEXCEPTION_POINTERS LPEXCEPTION_POINTERS;
#endif

struct _KTRAP_FRAME;
struct _KEXCEPTION_FRAME;
typedef struct _KTRAP_FRAME *PKTRAP_FRAME;
typedef struct _KEXCEPTION_FRAME *PKEXCEPTION_FRAME;
typedef BOOLEAN (__stdcall *PKDEBUG_ROUTINE)(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN BOOLEAN SecondChance
    );

#ifndef FORCEINLINE
#define FORCEINLINE static __inline
#endif

#ifndef LPCCH
typedef CONST CHAR *LPCCH, *PCCH;
#endif

#ifndef POSTR
typedef PSTR POSTR;
#endif

#if defined(RXDK_LIBXAPI_BUILD)
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
#define try if (1)
#define except(x) else if (0)
#define leave
#define finally if (1)
#ifndef GetExceptionCode
#define GetExceptionCode() ((DWORD)0)
#endif
#else
#ifndef try
#define try __try
#define except(x) __except(x)
#define leave __leave
#define finally __finally
#endif
#endif

#include <string.h>

#ifndef RtlCopyMemory
#define RtlCopyMemory(D, S, L) memcpy((D), (S), (L))
#endif
#ifndef RtlMoveMemory
#define RtlMoveMemory(D, S, L) memmove((D), (S), (L))
#endif
#ifndef RtlZeroMemory
#define RtlZeroMemory(D, L) memset((D), 0, (L))
#endif
#ifndef RtlFillMemory
#define RtlFillMemory(D, L, F) memset((D), (F), (L))
#endif

#ifndef NtCurrentProcess
#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
#endif
#ifndef NtCurrentThread
#define NtCurrentThread() ((HANDLE)(LONG_PTR)-2)
#endif

#ifndef DUPLICATE_SAME_ACCESS
#define DUPLICATE_SAME_ACCESS 0x00000002
#endif

#ifndef RtlDeleteCriticalSection
#define RtlDeleteCriticalSection(CriticalSection) ((void)(CriticalSection))
#endif

