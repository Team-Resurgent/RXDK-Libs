/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

// Compiles the source in guids.cpp into a distinctly named object file
// (guids_dmscript.obj), avoiding a name clash when linking dmusic.lib.

//#define INITGUID
/*#include "dmusicip.h"
#include "dmscriptautguids.h"
#include "dmstrm.h"
#include "..\shared\dmusicp.h"*/

#include "pchscript.h"

