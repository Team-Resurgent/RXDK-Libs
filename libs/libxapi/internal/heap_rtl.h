#ifndef RXDK_XAPI_HEAP_RTL_H
#define RXDK_XAPI_HEAP_RTL_H

/*
 * libxapi rtl heap entry points (vendor rtl/heap.c). Title TUs need these
 * before winbase.h maps HeapAlloc -> RtlAllocateHeap.
 */

/*
 * These are C entry points implemented in rtl/heap.c, so a C++ TU that reaches
 * this header without an extern "C" guard references mangled names libxapi does
 * not export. That is what broke the DirectMusic samples: dmime/debug.cpp's
 * HeapAlloc/HeapSize expanded to RtlAllocateHeap/RtlSizeHeap and it emitted
 * __Z15RtlAllocateHeapPvmj@12. Guarded here rather than at each include site --
 * every other libxapi header that declares these already does the same.
 */
#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
