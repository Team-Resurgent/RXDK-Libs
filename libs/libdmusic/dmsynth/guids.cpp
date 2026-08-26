/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * GUID definition unit for the synthesizer. Includes initguid.h and the
 * DirectMusic/DirectSound headers so the class and interface IDs used by the
 * synth are emitted into this single object file.
 */

#ifdef XBOX
#include <xtl.h>
#define COM_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#define _WINGDI_
#endif

#include <objbase.h>
#include "initguid.h"
#include <mmsystem.h>
#include <dsoundp.h>
#include "dmusicc.h"
#include "dmusics.h"
#include "synth.h"
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
#include "dmusicip.h"
#include "dsoundp.h"
#include "..\shared\dmusiccp.h" // For class ids.
#if 0 // The following section will only take affect in the DDK sample.
// @@END_DDKSPLIT
#include <ks.h>
#include "umsynth.h"
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
#endif
// @@END_DDKSPLIT
#include "dmksctrl.h"
