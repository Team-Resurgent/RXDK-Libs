/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Umbrella header that pulls in the core Windows SDK headers (windef.h,
 * winbase.h, wingdi.h, winerror.h and related) for this library's build.
 */

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef _INC_WINDOWS
#define _INC_WINDOWS

#if defined(RC_INVOKED)

#include <winresrc.h>

#else  // RC_INVOKED

#define NOD3D
#define NODSOUND

#include <xtl.h>

#undef  NOD3D
#undef  NODSOUND

#include <wingdi.h>

#endif // RC_INVOKED

#endif  /* _INC_WINDOWS */


