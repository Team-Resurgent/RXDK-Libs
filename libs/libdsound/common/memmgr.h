/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * DirectSound memory-manager interface.
 */

#ifndef __MEMMGR_H__
#define __MEMMGR_H__

#include "macros.h"

#if defined(DEBUG) && !defined(TRACK_MEMORY_USAGE) && !defined(MCPX_BOOT_LIB)
#define TRACK_MEMORY_USAGE
#endif // defined(DEBUG) && !defined(TRACK_MEMORY_USAGE) && !defined(MCPX_BOOT_LIB)

BEGIN_DEFINE_ENUM()
    DSOUND_OBJECT_POOL_TAG  = 'boSD',
    DSOUND_DATA_POOL_TAG    = 'adSD'
END_DEFINE_ENUM(DSOUND_POOL_TAG);

BEGIN_DEFINE_ENUM()
    DSOUND_ALLOCATOR_POOL   = 'loop',
    DSOUND_ALLOCATOR_PHYS   = 'syhp',
    DSOUND_ALLOCATOR_SLOP   = 'pols'
END_DEFINE_ENUM(DSOUND_ALLOCATOR_TAG);

//
// Allocation tracking data
//

BEGIN_DEFINE_STRUCT()
    LIST_ENTRY              leListEntry;
    LPCSTR                  pszFile;
    ULONG                   nLine;
    LPCSTR                  pszClass;
    DSOUND_ALLOCATOR_TAG    nAllocatorTag;
    ULONG                   cbSize;
    LPVOID                  pvBaseAddress;
END_DEFINE_STRUCT(DSMEMTRACK);

#ifdef __cplusplus

//
// Memory manager
//

namespace DirectSound
{
    class CMemoryManager
    {
    public:

#ifdef BUILDING_DSOUND

        static DWORD &      m_dwPoolMemoryUsage;            // Pool memory usage, in bytes
        static DWORD &      m_dwPhysicalMemoryUsage;        // Physical memory usage, in bytes

#else // BUILDING_DSOUND

        static DWORD        m_dwPoolMemoryUsage;            // Pool memory usage, in bytes
        static DWORD        m_dwPhysicalMemoryUsage;        // Physical memory usage, in bytes

#endif // BUILDING_DSOUND

#ifdef TRACK_MEMORY_USAGE

    protected:
        static LIST_ENTRY   m_lstMemoryTracking;            // Allocation list

#endif // TRACK_MEMORY_USAGE
    
    public:
        // Pool memory
        static LPVOID PoolAlloc(DSOUND_POOL_TAG nTag, ULONG cbBuffer, BOOL fZeroInit);
        static void PoolFree(LPVOID pvBuffer);

        // Physically contiguous memory
        static LPVOID PhysicalAlloc(ULONG cbBuffer, ULONG cbAlignment, DWORD dwFlags, BOOL fZeroInit);
        static void PhysicalFree(LPVOID pvBuffer);

#ifdef ENABLE_SLOP_MEMORY_RECOVERY

        // Unused memory recovery
        static LPVOID MemAlloc(DSOUND_POOL_TAG nTag, ULONG cbBuffer, BOOL fZeroInit);
        static void MemFree(LPVOID pvBuffer);

#endif // ENABLE_SLOP_MEMORY_RECOVERY

#ifdef TRACK_MEMORY_USAGE

        // Usage tracking
        static LPVOID TrackingPoolAlloc(LPCSTR pszFile, ULONG nLine, LPCSTR pszClass, DSOUND_POOL_TAG nTag, ULONG cbBuffer, BOOL fZeroInit);
        static void TrackingPoolFree(LPVOID pvBuffer);

        static LPVOID TrackingPhysicalAlloc(LPCSTR pszFile, ULONG nLine, LPCSTR pszClass, ULONG cbBuffer, ULONG cbAlignment, DWORD dwFlags, BOOL fZeroInit);
        static void TrackingPhysicalFree(LPVOID pvBuffer);

#ifdef ENABLE_SLOP_MEMORY_RECOVERY

        static LPVOID TrackingMemAlloc(LPCSTR pszFile, ULONG nLine, LPCSTR pszClass, DSOUND_POOL_TAG nTag, ULONG cbBuffer, BOOL fZeroInit);
        static void TrackingMemFree(LPVOID pvBuffer);

#endif // ENABLE_SLOP_MEMORY_RECOVERY

#endif // TRACK_MEMORY_USAGE

        static void DumpMemoryUsage(BOOL fAssertNone);

#ifdef TRACK_MEMORY_USAGE

    private:
        // Tracking information
        static LPVOID TrackAlloc(LPVOID pvBaseAddress, ULONG cbTracking, LPCSTR pszFile, ULONG nLine, LPCSTR pszClass, DSOUND_ALLOCATOR_TAG nAllocatorTag, ULONG cbSize);
        static LPVOID ForgetAlloc(LPVOID pvBaseAddress, DSOUND_ALLOCATOR_TAG nAllocatorTag);

#endif // TRACK_MEMORY_USAGE

    };
}

#ifndef ENABLE_SLOP_MEMORY_RECOVERY

#define MemAlloc PoolAlloc
#define MemFree PoolFree

#ifdef TRACK_MEMORY_USAGE

#define TrackingMemAlloc TrackingPoolAlloc
#define TrackingMemFree TrackingPoolFree

#endif // TRACK_MEMORY_USAGE

#endif // ENABLE_SLOP_MEMORY_RECOVERY

#ifndef TRACK_MEMORY_USAGE

#define TrackingPoolAlloc(pszFile, nLine, pszClass, nTag, cbBuffer, fZeroInit) \
    PoolAlloc(nTag, cbBuffer, fZeroInit)

#define TrackingPoolFree(pvBuffer) \
    PoolFree(pvBuffer)

#define TrackingPhysicalAlloc(pszFile, nLine, pszClass, cbBuffer, cbAlignment, dwFlags, fZeroInit) \
    PhysicalAlloc(cbBuffer, cbAlignment, dwFlags, fZeroInit)

#define TrackingPhysicalFree(pvBuffer) \
    PhysicalFree(pvBuffer)

#define TrackingMemAlloc(pszFile, nLine, pszClass, nTag, cbBuffer, fZeroInit) \
    MemAlloc(nTag, cbBuffer, fZeroInit)

#define TrackingMemFree(pvBuffer) \
    MemFree(pvBuffer)

#endif // TRACK_MEMORY_USAGE

//
// Memory management macros
//

#define MEMALLOC(type, count) \
    ((type *)DirectSound::CMemoryManager::TrackingMemAlloc(__FILE__, __LINE__, #type, DSOUND_DATA_POOL_TAG, sizeof(type) * (count), TRUE))

#define MEMALLOC_NOINIT(type, count) \
    ((type *)DirectSound::CMemoryManager::TrackingMemAlloc(__FILE__, __LINE__, #type, DSOUND_DATA_POOL_TAG, sizeof(type) * (count), FALSE))

#define MEMFREE(p) \
    { \
        if(p) \
        { \
            DirectSound::CMemoryManager::TrackingMemFree(p); \
            (p) = NULL; \
        } \
    }

#define POOLALLOC(type, count) \
    ((type *)DirectSound::CMemoryManager::TrackingPoolAlloc(__FILE__, __LINE__, #type, DSOUND_DATA_POOL_TAG, sizeof(type) * (count), TRUE))

#define POOLALLOC_NOINIT(type, count) \
    ((type *)DirectSound::CMemoryManager::TrackingPoolAlloc(__FILE__, __LINE__, #type, DSOUND_DATA_POOL_TAG, sizeof(type) * (count), FALSE))

#define POOLFREE(p) \
    { \
        if(p) \
        { \
            DirectSound::CMemoryManager::TrackingPoolFree(p); \
            (p) = NULL; \
        } \
    }

#define PHYSALLOC(type, count, alignment, flags) \
    ((type *)DirectSound::CMemoryManager::TrackingPhysicalAlloc(__FILE__, __LINE__, #type, sizeof(type) * (count), alignment, flags, TRUE))

#define PHYSALLOC_NOINIT(type, count, alignment, flags) \
    ((type *)DirectSound::CMemoryManager::TrackingPhysicalAlloc(__FILE__, __LINE__, #type, sizeof(type) * (count), alignment, flags, FALSE))

#define PHYSFREE(p) \
    { \
        if(p) \
        { \
            DirectSound::CMemoryManager::TrackingPhysicalFree(p); \
            (p) = NULL; \
        } \
    }

//
// New and delete overrides
//

#if defined(DSOUND_NO_OVERRIDE_NEW_DELETE) && defined(TRACK_MEMORY_USAGE)
// The tracking build's NEW/NEW_A macros call the placement forms below, which
// live inside this block, and DELETE/DELETE_A call the plain global delete --
// so turning the overrides off while tracking is on would allocate from the
// DirectSound pool and free through libcpp. Pick one.
#error TRACK_MEMORY_USAGE requires the DirectSound operator new/delete overrides
#endif

#ifndef DSOUND_NO_OVERRIDE_NEW_DELETE

inline void *__cdecl operator new(size_t cbBuffer) 
{ 
    return DirectSound::CMemoryManager::TrackingMemAlloc("(none)", 0, "(unknown)", DSOUND_OBJECT_POOL_TAG, cbBuffer, TRUE); 
}

inline void *__cdecl operator new[](size_t cbBuffer) 
{ 
    return DirectSound::CMemoryManager::TrackingMemAlloc("(none)", 0, "(unknown)", DSOUND_OBJECT_POOL_TAG, cbBuffer, TRUE);
}

#ifdef TRACK_MEMORY_USAGE

inline void *__cdecl operator new(size_t cbBuffer, LPCSTR pszFile, ULONG nLine, LPCSTR pszClass)
{
    return DirectSound::CMemoryManager::TrackingMemAlloc(pszFile, nLine, pszClass, DSOUND_OBJECT_POOL_TAG, cbBuffer, TRUE); 
}

inline void *__cdecl operator new[](size_t cbBuffer, LPCSTR pszFile, ULONG nLine, LPCSTR pszClass)
{
    return DirectSound::CMemoryManager::TrackingMemAlloc(pszFile, nLine, pszClass, DSOUND_OBJECT_POOL_TAG, cbBuffer, TRUE); 
}

#endif // TRACK_MEMORY_USAGE

inline void __cdecl operator delete(void *pvBuffer) 
{ 
    DirectSound::CMemoryManager::TrackingMemFree(pvBuffer); 
}

inline void __cdecl operator delete[](void *pvBuffer) 
{ 
    DirectSound::CMemoryManager::TrackingMemFree(pvBuffer); 
}

#endif // DSOUND_NO_OVERRIDE_NEW_DELETE

#ifdef DSOUND_NO_OVERRIDE_NEW_DELETE

//
// DirectSound's constructors only assign the members that need a non-zero
// default and leave the rest -- register caches, status words, object
// pointers -- to arrive already cleared, which is what the pool allocator
// behind the overrides above did (CMcpxVoiceClient::Initialize asserts on
// it: ASSERT(!m_RegCache.CfgFMT)). With the overrides off the title owns
// `new`, so keep that contract with a tagged allocation form of our own
// rather than by taking the global operator back.
//

enum DSOUND_ZEROED_TAG { DsZeroed };

inline void *__cdecl operator new(size_t cbBuffer, DSOUND_ZEROED_TAG)
{
    void *pvBuffer = ::operator new(cbBuffer);

    if(pvBuffer)
    {
        memset(pvBuffer, 0, cbBuffer);
    }

    return pvBuffer;
}

inline void *__cdecl operator new[](size_t cbBuffer, DSOUND_ZEROED_TAG)
{
    void *pvBuffer = ::operator new[](cbBuffer);

    if(pvBuffer)
    {
        memset(pvBuffer, 0, cbBuffer);
    }

    return pvBuffer;
}

//
// Only reached if a constructor throws, which this build cannot do.
//

inline void __cdecl operator delete(void *pvBuffer, DSOUND_ZEROED_TAG)
{
    ::operator delete(pvBuffer);
}

inline void __cdecl operator delete[](void *pvBuffer, DSOUND_ZEROED_TAG)
{
    ::operator delete[](pvBuffer);
}

#define NEW(type) \
    new(DsZeroed) type

#define NEW_A(type, count) \
    new(DsZeroed) type [count]

#elif defined(TRACK_MEMORY_USAGE)

#define NEW(type) \
    new(__FILE__, __LINE__, #type) type

#define NEW_A(type, count) \
    new(__FILE__, __LINE__, #type) type [count]

#else // TRACK_MEMORY_USAGE

#define NEW(type) \
    new type

#define NEW_A(type, count) \
    new type [count]

#endif // DSOUND_NO_OVERRIDE_NEW_DELETE

#undef DELETE
#define DELETE(p) \
    { \
        if(p) \
        { \
            delete (p); \
            (p) = NULL; \
        } \
    }

#define DELETE_A(p) \
    { \
        if(p) \
        { \
            delete [] (p); \
            (p) = NULL; \
        } \
    }

#endif // __cplusplus

#endif // __MEMMGR_H__
