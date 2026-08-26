/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#pragma once
#define RXDK_UUID_BRIDGE_H

/* uuid slice builds COM/RPC IDL against the SDK. */
#ifndef XAPI_UUID_BUILD
#define XAPI_UUID_BUILD 1
#endif



/*
 * Force-included bridge header for the libxapi uuid slice (COM/RPC IDL objects).
 * sdk/winbase.h omits Interlocked* decls when _NTOS_ is set (k32 provides them).
 */

#ifndef _NTOS_
#define _NTOS_
#endif

