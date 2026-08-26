/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Defines _tls_index, the runtime TLS slot index for the process. The full PE
 * TLS image template is built separately in xapi_tls_image.c.
 */

#include "bridge_k32.h"

#include "dllp.h"

ULONG _tls_index = 0;
