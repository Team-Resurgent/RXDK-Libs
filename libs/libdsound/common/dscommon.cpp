/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Common DirectSound support code shared across the library (aggregates the
 * shared implementation translation units).
 */

#include "dscommon.h"
#include "debug.cpp"
#include "memmgr.cpp"
#include "format.cpp"
#include "waveldr.cpp"
#include "drvhlp.cpp"
#include "imaadpcm.cpp"
