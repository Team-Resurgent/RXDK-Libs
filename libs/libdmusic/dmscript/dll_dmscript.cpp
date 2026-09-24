/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

// Compiles the source in dll.cpp into a distinctly named object file
// (dll_dmscript.obj), avoiding a name clash when linking dmusic.lib.

#include "pchscript.h"
#include "dll.cpp"
