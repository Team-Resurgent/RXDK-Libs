/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Memory-allocation convenience macros (MemAlloc, MemAllocNoZero, MemReAlloc,
 * MemFree) wrapping the Xbox Local* heap allocator, which is backed by
 * RtlAllocateHeap on the process heap and provides 8-byte alignment.
 */

#ifndef __MEMALLOC_INCLUDED__
#define __MEMALLOC_INCLUDED__

// Note that RtlAllocateHeap(XapiProcessHeap()), which is what LocalAlloc
// amounts to on Xbox, has 8 byte alignment:

#define MemAlloc(size) ((VOID*) LocalAlloc(LMEM_ZEROINIT, size))
#define MemAllocNoZero(size) ((VOID*) LocalAlloc(0, size))
#define MemReAlloc(p, size) ((VOID*) LocalReAlloc(p, size, LMEM_ZEROINIT))
#define MemFree(p) LocalFree((p))

#endif
