/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * GUID storage for DirectMusic. Compiled with INITGUID so that including the
 * DirectMusic headers (via the dmime precompiled header) turns every DEFINE_GUID
 * into an actual definition, giving the whole component its CLSID/IID storage in
 * one translation unit.
 */

#define INITGUID
#include "..\dmime\pchime.h"




