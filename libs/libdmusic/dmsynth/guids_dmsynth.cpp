/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Thin wrapper that recompiles guids.cpp under a distinct object name
 * (guids_dmsynth.obj) so the synth's GUIDs can be linked without colliding
 * with the copy already in dmusic.lib.
 */

#include "guids.cpp"