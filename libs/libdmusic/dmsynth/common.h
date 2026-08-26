/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Common precompiled include set for the kernel-mode (miniport) build of the
 * DirectMusic software synthesizer. Pulls in the PortCls, KS and DLS headers
 * plus the synth's own core headers, and sets the default debug verbosity.
 */
#ifndef _COMMON_H_
#define _COMMON_H_


#if (DBG)
#if !defined(DEBUG_LEVEL)
#define DEBUG_LEVEL DEBUGLVL_VERBOSE
#endif
#endif

#include <winerror.h>


#include "portcls.h"
#include "ksdebug.h"
#include <dmusicks.h>       // Ks defines
#include <dmerror.h>        // Error codes
#include <dmdls.h>          // DLS definitions

#include "kernhelp.h"
#include "CSynth.h"
#include "synth.h"
#include "float.h"
#include "muldiv32.h"
#include "SysLink.h"

#endif  //_COMMON_H_
