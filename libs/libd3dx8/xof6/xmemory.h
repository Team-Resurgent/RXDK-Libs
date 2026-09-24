/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Declares the XMalloc, XRealloc and XFree memory-allocation helpers.
 */

#ifndef _XMEMORY_H_
#define _XMEMORY_H_

HRESULT XMalloc(void ** p_out, size_t size);
HRESULT XRealloc(void** p_inout, size_t size);
void    XFree(void *p);

#if defined(__cplusplus)

    /* RXDK: a global operator new/delete cannot be `static` in C++ (clang
       errors; MSVC tolerated it). __forceinline already gives inline linkage. */
    __forceinline void* operator new(size_t size)
    {
        void *p;
        XMalloc(&p, size);
        return p;
    }

    __forceinline void operator delete(void* p)
    {
        XFree(p);
    }

#endif

#endif // _XMEMORY_H_
