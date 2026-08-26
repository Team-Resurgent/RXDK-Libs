/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * XACT memory manager interface -- the allocation entry points the runtime
 * routes all pool allocations through (XactMemAlloc/Free and the tracking
 * XactTrackMemAlloc/Free), plus the live-usage counter.
 */

#ifndef __MEMMGR_H__
#define __MEMMGR_H__

#include "macros.h"

EXTERN_C LPVOID XactTrackMemAlloc(LPCSTR pszFile, ULONG nLine, LPCSTR pszClass, ULONG cbBuffer, BOOL fZeroInit);
EXTERN_C void XactTrackMemFree(LPVOID pvBuffer);

EXTERN_C LPVOID XactMemAlloc(ULONG cbBuffer, BOOL fZeroInit);
EXTERN_C void XactMemFree(LPVOID pvBuffer);

// Live pool usage, reported by IXACTEngine_GetRealtimeData (retail keeps the
// same running total in g_dwXACTEngineMemoryUsage).
EXTERN_C volatile LONG g_lXactMemoryUsage;


#endif // __MEMMGR_H__
