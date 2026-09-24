/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Master internal header for the DirectSound implementation: pulls in the
 * kernel and DirectSound headers and the shared internal declarations.
 */

#ifndef __DSOUNDI_H__
#define __DSOUNDI_H__

//
// Enable recovery of unused physical memory
//

#define ENABLE_SLOP_MEMORY_RECOVERY

//
// Put all code and data into a DSOUND section
//

#pragma code_seg("DSOUND")
#pragma data_seg("DSOUND_RW")
#pragma const_seg("DSOUND_RD")
#pragma bss_seg("DSOUND_URW")

//
// Common includes
//

#include "dscommon.h"

//
// Missing types
//

DEFINEREFTYPE(DS3DLISTENER);
DEFINEREFTYPE(DS3DBUFFER);
DEFINEREFTYPE(DSI3DL2LISTENER);
DEFINEREFTYPE(DSI3DL2BUFFER);
DEFINEREFTYPE(XMEDIAPACKET);
DEFINELPCTYPE(LONG);
DEFINELPCTYPE(DWORD);

//
// Private includes
//

#include "dsmath.h"
#include "heap.h"
#include "hrtf.h"
#include "i3dl2.h"
#include "ac97.h"
#include "dsp.h"
#include "cipher.h"
#include "dsapi.h"
#include "dsperf.h"
#include "mcpxcore.h"
#include "wavexmo.h"
#include "ac97xmo.h"

#endif // __DSOUNDI_H__
