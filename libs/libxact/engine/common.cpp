/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Shared translation unit for the XACT engine. Pulls in the common engine
 * headers and documents the memory-allocation policy used across the library.
 */

#include "stdafx.h"

#include "common.h"

//
// XACT does not define a global replacement operator new/delete. A global
// replacement would collide with the CRT's own operators and produce a
// duplicate-symbol link error, so XACT's C++ objects use the CRT operator new.
// XactMemAlloc/XactMemFree remain in use for XACT's own internal buffer
// allocations, which are pooled via ExAllocatePoolWithTag.
//
