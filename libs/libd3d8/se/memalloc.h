/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Memory allocation declarations for the Direct3D driver.
 */

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{

#if STARTUPANIMATION

// When building the startup animation, both the debug and the retail
// allocation functions get mapped to the same thing:

void *MemAlloc(DWORD Size);
void *MemAllocNoZero(DWORD Size);
void MemFree(void *p);

#elif DBG || PROFILE

// For normal debug builds and profile builds, we thunk the allocation calls:

void *MemAlloc(DWORD Size);
void *MemAllocNoZero(DWORD Size);
void MemFree(void *p);

#else

// For normal retail builds, we do the allocations inline:

#define MemAlloc(size) ((VOID*) LocalAlloc(LMEM_ZEROINIT, size))
#define MemAllocNoZero(size) ((VOID*) LocalAlloc(0, size))
#define MemFree(p) LocalFree((p))

#endif 

} // end namespace
