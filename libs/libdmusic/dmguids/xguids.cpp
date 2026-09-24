/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * GUID storage for DirectMusic. Compiled with INITGUID so that including the
 * DirectMusic headers (via the dmime precompiled header) turns every DEFINE_GUID
 * into an actual definition, giving the whole component its CLSID/IID storage in
 * one translation unit.
 */

#define INITGUID
#include "..\dmime\pchime.h"




