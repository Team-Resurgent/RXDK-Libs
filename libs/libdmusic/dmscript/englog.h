/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// Helper functions for logging script parsing.  Useful for debugging, but never turned on in released builds.

#error This file should never be used in released builds. // §§

#pragma once

#include "englex.h"
#include "engcontrol.h"

void LogToken(Lexer &l);
void LogRoutine(Script &script, Routines::index irtn);
