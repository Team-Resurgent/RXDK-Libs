/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * RtlFindFirstSetRightMember: returns the bit index of the lowest set bit in a
 * 32-bit set (0 when the set is empty). C reimplementation of the original
 * hand-written i386 bitmap assembly.
 */

#include "bridge_k32.h"

#include <xboxkrnl/xboxdef.h>

UCHAR __attribute__((fastcall)) RtlFindFirstSetRightMember(ULONG Set)
{
    if (Set == 0) {
        return 0;
    }

    for (int i = 0; i < 32; i++) {
        if (Set & (1UL << i)) {
            return (UCHAR)i;
        }
    }
    return 0;
}
