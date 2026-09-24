/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Precompiled header for the D3DX shape library: the common D3DX and Xbox
 * includes shared by the shape-generation sources, plus the RELEASE helper macro.
 */

#include <d3dx8seg.h>
#include <xtl.h>
#include "d3dx8dbg.h"
#include "malloc.h"

#define RELEASE(x) \
    do { if(x) { x->Release(); x = NULL; } } while(0)
