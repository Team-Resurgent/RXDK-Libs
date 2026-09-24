/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * No-op implementations of the RIP debug-assertion entry points (RIP,
 * RIP_ON_NOT_TRUE, RIP_ON_NOT_TRUE_WITH_MESSAGE) that the xdbg.h macros call in
 * retail libxapi builds, where these checks compile away to nothing.
 */

#include "bridge_k32.h"

#include <xboxkrnl/xboxdef.h>

void RIP(void)
{
}

void RIP_ON_NOT_TRUE(const char *api, int expr)
{
    (void)api;
    (void)expr;
}

void RIP_ON_NOT_TRUE_WITH_MESSAGE(int expr, const char *msg)
{
    (void)expr;
    (void)msg;
}
