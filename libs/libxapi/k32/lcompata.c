#include "bridge_k32.h"
/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * ANSI build of the lstr* compatibility string functions. Defines LCOMPATA and
 * includes lcompat.c so the shared implementation compiles in its 'A' form.
 */

#include "basedll.h"
#pragma hdrstop

#define LCOMPATA
#include "lcompat.c"

