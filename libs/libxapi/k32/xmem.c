#include "bridge_k32.h"
/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * XMemAlloc / XMemFree / XMemSize - the allocator entry points xAPI uses for its
 * own internal allocations, and which a title may replace to route those
 * allocations through an allocator of its own.
 *
 * Overriding works by the ordinary static-link rule: the linker only pulls an
 * archive member in to satisfy a symbol that is still undefined, so when the
 * title defines XMemAlloc itself, this member is never pulled and there is no
 * duplicate-symbol error. That only holds while this member carries nothing
 * else a title needs - which is why these three forwarders sit alone here
 * rather than in xapiheap.c beside HeapCreate and LocalAlloc, and why the
 * implementations they forward to live in xmemdflt.c rather than inline.
 */

#include "basedll.h"
#pragma hdrstop

LPVOID
__attribute__((__stdcall__))
XMemAlloc(
    SIZE_T dwSize,
    DWORD dwAllocAttributes
    )
{
    return XMemAllocDefault(dwSize, dwAllocAttributes);
}

VOID
__attribute__((__stdcall__))
XMemFree(
    PVOID pAddress,
    DWORD dwAllocAttributes
    )
{
    XMemFreeDefault(pAddress, dwAllocAttributes);
}

SIZE_T
__attribute__((__stdcall__))
XMemSize(
    PVOID pAddress,
    DWORD dwAllocAttributes
    )
{
    return XMemSizeDefault(pAddress, dwAllocAttributes);
}
