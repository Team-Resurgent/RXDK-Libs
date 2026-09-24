/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * NT-style driver helper functions and objects for the DirectSound build
 * (aggregates the shared implementation).
 */

#include "dsoundi.h"

DWORD CFpState::m_dwRefCount = 0;
KFLOATING_SAVE CFpState::m_fps;


