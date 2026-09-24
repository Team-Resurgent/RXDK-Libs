/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Defines _tls_array, the byte offset of the TLS array within the thread
 * information block (TIB.StackBase). C reimplementation of the original i386
 * assembly stub.
 */

#include "bridge_k32.h"

const int _tls_array = 4;
