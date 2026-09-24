/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Umbrella include for the DirectMusic composition engine. Pulls in the
 * DirectMusic interfaces and file formats plus this component's own helpers
 * (string, template list, RIFF stream, template structures and the SuperJam
 * personality definitions) so translation units get one header to include.
 */

#ifndef __COMPOSINH__
#define __COMPOSINH__

#include "dmusicip.h"
#include "dmusicf.h"
#include "str.h"
#include "tlist.h"

#include "aariff.h"
#include "templats.h"

#include "sjpers.h"

#include "debug.h"

#endif
