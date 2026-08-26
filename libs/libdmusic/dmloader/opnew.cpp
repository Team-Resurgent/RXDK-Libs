/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Overrides operator new[] / delete[] for the loader so allocation bypasses the
 * new_handler mechanism and simply returns NULL on failure.
 */

#ifdef XBOX
#include <xtl.h>
#else // XBOX
#include <windows.h>
#endif // XBOX

#include <stdio.h>
#include <stdlib.h>

#include "Debug.h"

#ifndef DMUSIC_NO_OVERRIDE_NEW_DELETE

LPVOID __cdecl operator new(size_t cbBuffer)
{
    LPVOID p;

    p = malloc(cbBuffer ? cbBuffer : 1);
    return p;
}

void __cdecl operator delete(LPVOID p)
{
    free(p);
}

#endif // DMUSIC_NO_OVERRIDE_NEW_DELETE

