/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

// Helper functions for logging script parsing.  Useful for debugging, but never turned on in released builds.

#error This file should never be used in released builds. // §§

#pragma once

#include "englex.h"
#include "engcontrol.h"

void LogToken(Lexer &l);
void LogRoutine(Script &script, Routines::index irtn);
