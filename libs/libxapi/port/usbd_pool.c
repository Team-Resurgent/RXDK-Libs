/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * USBD pool allocator (USBD_AllocateMemory / USBD_FreeMemory), which the USB
 * stack declares but does not define. Implemented over ExAllocatePool /
 * ExFreePool with the cdecl linkage the usbd and tree call sites expect.
 */

#include "bridge_usb.h"

#include <ntos.h>

PVOID USBD_AllocateMemory(ULONG cb, ULONG Tag)
{
    (void)Tag;
    return ExAllocatePool((SIZE_T)cb);
}

VOID USBD_FreeMemory(PVOID pv)
{
    ExFreePool(pv);
}
