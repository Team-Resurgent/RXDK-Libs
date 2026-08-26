/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#ifndef _RXDK_DMUSIC_WINDOWS_H_
#define _RXDK_DMUSIC_WINDOWS_H_
/*
 * windows.h shim for the Xbox DirectMusic port. A few of the original's shared
 * helper headers (xprivate/sem32.hxx, the OLE32-derived memstm) reach for the
 * full <windows.h>. On Xbox that surface is xtl.h (Win32 + xapi), which the
 * bridge already force-includes; this shim resolves <windows.h> to that plus the
 * base COM/OLE headers, shadowing zig's MinGW windows.h (which would pull the
 * whole win32 header set and fail on this freestanding target).
 */
#include <xtl.h>
#include <objbase.h>
#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif

#endif /* _RXDK_DMUSIC_WINDOWS_H_ */
