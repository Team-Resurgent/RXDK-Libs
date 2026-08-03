//
// Copyright (C) Microsoft Corporation. All Rights Reserved.
//
#pragma once

extern void* DirectMusicAllocI(size_t cb);
extern void  DirectMusicFreeI(void *pv);
extern void* DirectMusicPhysicalAllocI(size_t dwSize);
extern void DirectMusicPhysicalFreeI(void* lpAddress);

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
