/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#ifndef RXDK_XAPI_HEAP_RTL_H
#define RXDK_XAPI_HEAP_RTL_H

/*
 * libxapi rtl heap entry points (implemented in rtl/heap.c). Title translation
 * units need these before winbase.h maps HeapAlloc to RtlAllocateHeap.
 *
 * These are C entry points, so a C++ TU that reaches this header without an
 * extern "C" guard would reference mangled names libxapi does not export. The
 * extern "C" guard below prevents that (for example dmime/debug.cpp expanding
 * HeapAlloc/HeapSize to RtlAllocateHeap/RtlSizeHeap and emitting a mangled
 * __Z15RtlAllocateHeapPvmj@12). Every other libxapi header that declares these
 * guards them the same way.
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
