/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// Compiles the source in xguids.cpp into a distinctly named object file
// (xguids_dmscript.obj), avoiding a name clash when linking dmusic.lib.

#include "pchscript.h"
#include "xguids.cpp"