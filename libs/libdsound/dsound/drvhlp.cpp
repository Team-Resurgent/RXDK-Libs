/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * NT-style driver helper functions and objects for the DirectSound build
 * (aggregates the shared implementation).
 */

#include "dsoundi.h"

DWORD CFpState::m_dwRefCount = 0;
KFLOATING_SAVE CFpState::m_fps;


