#ifndef RXDK_RTL_BRIDGE_H
#define RXDK_RTL_BRIDGE_H

/* Force-included for libxapi rtl slice (sdk nt.h path, not xboxkrnl). */

#ifndef NTKERNELAPI
#define NTKERNELAPI
#endif
#ifndef NTHALAPI
#define NTHALAPI
#endif

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
/* Vendor rtl/heap.c uses MSVC SEH (try/leave/finally). leave must jump past the
 * error epilogue; an empty leave lets success paths fall through to ReturnValue=NULL. */
#define try if (1)
#define except(x) else if (0)
#define leave goto __rxdk_seh_finally
#define finally __rxdk_seh_finally: if (1)
#ifndef GetExceptionCode
#define GetExceptionCode() ((NTSTATUS)0)
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#ifndef PAGE_READWRITE
#define PAGE_READWRITE 0x04
#endif
#ifndef MM_HIGHEST_USER_ADDRESS
#define MM_HIGHEST_USER_ADDRESS ((PVOID)(ULONG_PTR)0x7FFEFFFF)
#endif
#ifndef MM_LOWEST_USER_ADDRESS
#define MM_LOWEST_USER_ADDRESS ((PVOID)(ULONG_PTR)0x00010000)
#endif

#ifndef SLIST_ENTRY_DEFINED
#define SLIST_ENTRY_DEFINED 1
typedef struct _SLIST_ENTRY {
    struct _SLIST_ENTRY *Next;
} SLIST_ENTRY, *PSLIST_ENTRY;
#endif

#ifndef SLIST_HEADER_DEFINED
#define SLIST_HEADER_DEFINED 1
typedef union _SLIST_HEADER {
    unsigned long long Alignment;
    struct {
        SLIST_ENTRY Next;
        unsigned short Depth;
        unsigned short Sequence;
    } Header8;
} SLIST_HEADER, *PSLIST_HEADER;
#endif

#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#ifndef NTKERNELAPI
#define NTKERNELAPI
#endif
#ifndef NTSYSAPI
#define NTSYSAPI
#endif
#ifndef NTHALAPI
#define NTHALAPI
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
#ifndef RtlInitObjectString
#define RtlInitObjectString RtlInitAnsiString
#endif

#endif
