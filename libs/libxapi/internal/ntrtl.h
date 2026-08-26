/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#ifndef RXDK_INCLUDE_NTRTL_H
#define RXDK_INCLUDE_NTRTL_H

/*
 * ntrtl.h include shim: private libxapi builds (_XAPIP_ / _BASEP_ / _DLLP_)
 * pull the local rxdk_ntrtl.h; everything else uses the SDK ntrtl_sdk.h.
 */

#if (defined(_XAPIP_) || defined(_BASEP_) || defined(_DLLP_)) && !defined(_NTRTLP_)
#include "rxdk_ntrtl.h"
#else
#include <ntrtl_sdk.h>
#endif

#endif
