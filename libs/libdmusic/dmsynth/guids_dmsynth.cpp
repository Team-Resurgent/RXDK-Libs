/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Thin wrapper that recompiles guids.cpp under a distinct object name
 * (guids_dmsynth.obj) so the synth's GUIDs can be linked without colliding
 * with the copy already in dmusic.lib.
 */

#include "guids.cpp"