/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Defines the per-fiber XFIBER structure - the fiber data pointer plus the
 * stack bookkeeping (base, limit, and saved kernel stack pointer) used by the
 * fiber services.
 */

#pragma once
#define _XFIBER_H


//
// Structure to hold the per fiber instance data.
//

typedef struct _XFIBER {
    PVOID FiberData;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID KernelStack;
} XFIBER, *PXFIBER;

