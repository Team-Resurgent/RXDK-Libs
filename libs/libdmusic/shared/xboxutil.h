/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * xboxutil.h -- Xbox compatibility helpers for the DirectMusic sources.
 * Restores real function forms of primitives the Xbox headers stub as empty
 * macros (for example DeleteCriticalSection) so member-call syntax keeps
 * compiling.
 */

#pragma once

#ifdef XBOX

// On Xbox, DeleteCriticalSection is defined as an empty macro.
// This causes problems when it is called in the DMusic sources
// as ::DeleteCriticalSection(...);
// which macro-expands to:
// ::(...);

#ifdef DeleteCriticalSection
#undef DeleteCriticalSection
inline void DeleteCriticalSection(void* /* unused */)
{
}
#endif // DeleteCriticalSection

#endif // XBOX