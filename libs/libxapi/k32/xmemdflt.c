#include "bridge_k32.h"
/*++

Copyright (c) 1990-1999  Microsoft Corporation

Module Name:

    xmemdflt.c

Abstract:

    xAPI's built-in allocator: XMemAllocDefault / XMemFreeDefault /
    XMemSizeDefault.

    These are always present. A title that replaces XMemAlloc & co. still needs
    them -- an override is nearly always a wrapper that tracks the request and
    then defers to the original behaviour, and it reaches that behaviour by
    calling these. CustomMemoryAllocator is exactly that shape.

    The overridable XMemAlloc/XMemFree/XMemSize forwarders live separately, in
    xmem.c; see the note there for why the split matters to the linker.

--*/

#include "basedll.h"
#pragma hdrstop

LPVOID
__attribute__((__stdcall__))
XMemAllocDefault(
    SIZE_T dwSize,
    DWORD dwAllocAttributes
    )
{
    if (XALLOC_IS_PHYSICAL(dwAllocAttributes)) {
        return XPhysicalAlloc(dwSize, MAXULONG_PTR, 0, PAGE_READWRITE);
    }
    return LocalAlloc(LMEM_FIXED, dwSize);
}

VOID
__attribute__((__stdcall__))
XMemFreeDefault(
    PVOID pAddress,
    DWORD dwAllocAttributes
    )
{
    if (pAddress == NULL) {
        return;
    }
    if (XALLOC_IS_PHYSICAL(dwAllocAttributes)) {
        XPhysicalFree(pAddress);
    } else {
        LocalFree(pAddress);
    }
}

SIZE_T
__attribute__((__stdcall__))
XMemSizeDefault(
    PVOID pAddress,
    DWORD dwAllocAttributes
    )
{
    if (pAddress == NULL) {
        return 0;
    }
    if (XALLOC_IS_PHYSICAL(dwAllocAttributes)) {
        return XPhysicalSize(pAddress);
    }
    return LocalSize((HLOCAL)pAddress);
}
