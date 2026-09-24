#include "bridge_k32.h"
/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * ANSI build of the lstr* compatibility string functions. Defines LCOMPATA and
 * includes lcompat.c so the shared implementation compiles in its 'A' form.
 */

#include "basedll.h"
#pragma hdrstop

#define LCOMPATA
#include "lcompat.c"

