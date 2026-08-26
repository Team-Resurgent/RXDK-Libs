/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
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
