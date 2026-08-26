/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Miscellaneous NT-style driver helper functions and objects shared by the
 * DirectSound components.
 */

#include "dscommon.h"

#ifdef _XBOX

DWORD CFpState::m_dwRefCount = 0;
KFLOATING_SAVE CFpState::m_fps;

#endif // _XBOX

