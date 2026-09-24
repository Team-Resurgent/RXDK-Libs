/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * xalloc.h -- the DirectMusic allocator hooks. Declares DirectMusicAllocI /
 * DirectMusicFreeI and their physical-memory variants, and (unless suppressed)
 * overrides the global operator new/delete to route allocations through them.
 */

#pragma once

extern void* DirectMusicAllocI(size_t cb);
extern void  DirectMusicFreeI(void *pv);
extern void* DirectMusicPhysicalAllocI(size_t dwSize);
extern void DirectMusicPhysicalFreeI(void* lpAddress);

#ifndef DMUSIC_NO_OVERRIDE_NEW_DELETE

// RXDK/clang: a replacement global operator new/delete must have external
// linkage, so `static` (which MSVC accepted here) is rejected -- use `inline`
// (vague linkage, one merged definition across the TUs that include xalloc.h).
inline void * __cdecl operator new(size_t cb)
{
    return DirectMusicAllocI(cb);
}

inline void __cdecl operator delete(void *pv)
{
    DirectMusicFreeI(pv);
}

inline void * __cdecl operator new[](size_t cb)
{
    return DirectMusicAllocI(cb);
}

inline void __cdecl operator delete[](void *pv)
{
    DirectMusicFreeI(pv);
}

#endif // DMUSIC_NO_OVERRIDE_NEW_DELETE
