/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

#ifndef RXDK_INCLUDE_NTURTL_H
#define RXDK_INCLUDE_NTURTL_H

/*
 * nturtl.h include shim: private libxapi builds (_XAPIP_ / _BASEP_ / _DLLP_)
 * pull the local rxdk_nturtl.h; everything else uses the SDK nturtl_sdk.h.
 */

#if defined(_XAPIP_) || defined(_BASEP_) || defined(_DLLP_)
#include "rxdk_nturtl.h"
#else
#include <nturtl_sdk.h>
#endif

#endif
