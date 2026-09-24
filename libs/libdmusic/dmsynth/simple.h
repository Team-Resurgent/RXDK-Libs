/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Common include prologue for the user-mode synth build. Pulls in the platform
 * headers (xtl.h on Xbox, windows.h otherwise) and provides fallback typedefs
 * such as SECURITY_ATTRIBUTES so the shared synth core compiles on Xbox.
 */

#ifdef XBOX
#include <xtl.h>
#else // XBOX
#include <windows.h>
#endif // XBOX

#include <windowsx.h>
#include <stdio.h>
#include "misc.h"

#ifdef XBOX
#ifndef _SECURITY_ATTRIBUTES_
#define _SECURITY_ATTRIBUTES_
typedef struct  _SECURITY_ATTRIBUTES
    {
    DWORD nLength;
    /* [size_is] */ LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
    }	SECURITY_ATTRIBUTES;
#endif // !_SECURITY_ATTRIBUTES_
#endif
