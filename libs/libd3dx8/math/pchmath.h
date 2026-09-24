/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Precompiled header for the D3DX8 math module: sets up the section layout
 * (d3dx8seg.h), pulls in xtl.h and the matrix-stack declaration, and stubs out
 * the D3DXASSERT and DPF debug macros so the math sources build without the
 * debug tracing machinery.
 */

#include <d3dx8seg.h>
#include "xtl.h"
#include "cstack.h"

#define D3DXASSERT
#define DPF

