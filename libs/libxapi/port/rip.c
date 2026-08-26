/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
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
