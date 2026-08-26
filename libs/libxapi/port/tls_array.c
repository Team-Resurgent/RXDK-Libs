/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Defines _tls_array, the byte offset of the TLS array within the thread
 * information block (TIB.StackBase). C reimplementation of the original i386
 * assembly stub.
 */

#include "bridge_k32.h"

const int _tls_array = 4;
