#ifndef RXDK_XAPI_HEAP_RTL_H
#define RXDK_XAPI_HEAP_RTL_H

/*
 * libxapi rtl heap entry points (vendor rtl/heap.c). Title TUs need these
 * before winbase.h maps HeapAlloc -> RtlAllocateHeap.
 */

#ifndef NTAPI
#define NTAPI __attribute__((__stdcall__))
#endif

struct _RTL_HEAP_PARAMETERS;

PVOID NTAPI RtlCreateHeap(
    ULONG Flags,
    PVOID HeapBase,
    SIZE_T ReserveSize,
    SIZE_T CommitSize,
    PVOID Lock,
    struct _RTL_HEAP_PARAMETERS *Parameters);
PVOID NTAPI RtlDestroyHeap(PVOID HeapHandle);
PVOID NTAPI RtlAllocateHeap(PVOID HeapHandle, ULONG Flags, SIZE_T Size);
PVOID NTAPI RtlReAllocateHeap(PVOID HeapHandle, ULONG Flags, PVOID BaseAddress, SIZE_T Size);
BOOLEAN NTAPI RtlFreeHeap(PVOID HeapHandle, ULONG Flags, PVOID BaseAddress);
SIZE_T NTAPI RtlSizeHeap(PVOID HeapHandle, ULONG Flags, PVOID BaseAddress);

#endif
