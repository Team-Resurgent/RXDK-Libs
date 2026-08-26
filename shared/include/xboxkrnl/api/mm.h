/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Mm* kernel imports: the memory manager. Physical and virtual page allocation
 * on the Xbox's flat, un-paged address space: physically contiguous blocks
 * (needed for GPU/DMA buffers), general system memory, I/O-space mapping,
 * page-protection control, virtual-to-physical translation, and the debug-kit
 * heap. Sizes are in bytes and rounded up to whole pages.
 */

#ifndef XBOXKRNL_API_MM_H
#define XBOXKRNL_API_MM_H

/* Allocates NumberOfBytes of page-aligned, physically contiguous memory (usable
 * for GPU/DMA). Returns NULL on failure; free with MmFreeContiguousMemory. */
PVOID STDCALL MmAllocateContiguousMemory(
    IN SIZE_T NumberOfBytes);

/* As MmAllocateContiguousMemory, but constrains the physical range
 * [LowestAcceptableAddress, HighestAcceptableAddress], the address Alignment,
 * and the page Protect flags. */
PVOID STDCALL MmAllocateContiguousMemoryEx(
    IN SIZE_T NumberOfBytes,
    IN ULONG_PTR LowestAcceptableAddress,
    IN ULONG_PTR HighestAcceptableAddress,
    IN ULONG_PTR Alignment,
    IN ULONG Protect);

/* Allocates NumberOfBytes of (not necessarily contiguous) system memory with the
 * given page Protect flags. Free with MmFreeSystemMemory. */
PVOID STDCALL MmAllocateSystemMemory(
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect);

/* Reserves NumberOfBytes of the GPU instance memory region, reporting alignment
 * padding in *NumberOfPaddingBytes. Called once during graphics init. */
PVOID STDCALL MmClaimGpuInstanceMemory(
    IN SIZE_T NumberOfBytes,
    OUT SIZE_T* NumberOfPaddingBytes);

/* Allocates a NumberOfBytes kernel stack for a new thread; DebuggerThread flags
 * a debugger-owned thread. Paired with MmDeleteKernelStack. */
PVOID STDCALL MmCreateKernelStack(
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN DebuggerThread);

/* Debug-kit only: allocates NumberOfBytes from the debug memory region. */
PVOID STDCALL MmDbgAllocateMemory(
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect);

/* Debug-kit only: frees a MmDbgAllocateMemory block; returns bytes freed. */
ULONG STDCALL MmDbgFreeMemory(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes);

/* Debug-kit only: returns the count of free physical pages. */
PFN_COUNT STDCALL MmDbgQueryAvailablePages(void);

/* Debug-kit only: releases a temporary mapping created for debugger access. */
VOID STDCALL MmDbgReleaseAddress(
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque);

/* Debug-kit only: makes a page temporarily writable for the debugger, returning
 * a usable address and saving the original PTE in *Opaque. */
PVOID STDCALL MmDbgWriteCheck(
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque);

/* Frees a kernel stack allocated by MmCreateKernelStack. */
VOID STDCALL MmDeleteKernelStack(
    IN PVOID KernelStackBase,
    IN PVOID KernelStackLimit);

/* Frees a block from MmAllocateContiguousMemory[Ex]. */
VOID STDCALL MmFreeContiguousMemory(
    IN PVOID BaseAddress);

/* Frees NumberOfBytes from a MmAllocateSystemMemory block; returns bytes freed. */
ULONG STDCALL MmFreeSystemMemory(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes);

/* Translates a virtual BaseAddress to its physical address (needed to hand
 * buffers to the GPU/DMA engines). */
ULONG_PTR STDCALL MmGetPhysicalAddress(
    IN PVOID BaseAddress);

/* Memory-manager global state (page counts, region bases). */
XBAPI MMGLOBALDATA MmGlobalData;

/* Returns TRUE if VirtualAddress is currently mapped and accessible. */
BOOLEAN STDCALL MmIsAddressValid(
    IN PVOID VirtualAddress);

/* Locks (UnlockPages FALSE) or unlocks (TRUE) the pages spanning a buffer so
 * they cannot be reclaimed during DMA. */
VOID STDCALL MmLockUnlockBufferPages(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN UnlockPages);

/* Locks or unlocks a single physical page. */
VOID STDCALL MmLockUnlockPhysicalPage(
    IN ULONG_PTR PhysicalAddress,
    IN BOOLEAN UnlockPage);

/* Maps NumberOfBytes of device physical I/O space at PhysicalAddress into a
 * usable virtual address with the given Protect flags. */
PVOID STDCALL MmMapIoSpace(
    IN ULONG_PTR PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect);

/* Marks a contiguous block as persistent (Persist TRUE) so it survives a title
 * relaunch, or clears that flag. */
VOID STDCALL MmPersistContiguousMemory(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN Persist);

/* Returns the page-protection flags currently applied to VirtualAddress. */
ULONG STDCALL MmQueryAddressProtect(
    IN PVOID VirtualAddress);

/* Returns the byte size of the allocation that BaseAddress belongs to. */
SIZE_T STDCALL MmQueryAllocationSize(
    IN PVOID BaseAddress);

/* Fills MemoryStatistics with current memory-manager figures. */
NTSTATUS STDCALL MmQueryStatistics(
    IN OUT PMM_STATISTICS MemoryStatistics);

/* Changes the page protection of a region to NewProtect. */
VOID STDCALL MmSetAddressProtect(
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytes,
    IN ULONG NewProtect);

/* Tears down a mapping created by MmMapIoSpace. */
PVOID STDCALL MmUnmapIoSpace(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes);

#endif
