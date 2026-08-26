/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Precompiled header for the dmband component.
 *
 * Pulls in the DirectMusic public and private headers plus the shared stream,
 * critical-section and validation helpers needed throughout the band code,
 * selecting the Xbox (xtl.h) or desktop (objbase.h) include set as appropriate.
 */

#ifdef XBOX
#include <xtl.h>
#include "..\shared\critsec.h"
#include "..\shared\xcreate.h"
#include "PChMap.h"
#include "dmksctrl.h"
#include "dmusicc.h"
#include "dmusicip.h"
#include "dmusicf.h"
#include "..\shared\dmstrm.h"
#include "..\shared\validate.h"
#include "..\shared\dmusiccp.h"
#include "dmbandp.h"
#include "bandtrk.h"
#include "debug.h"
#include "..\shared\xsoundp.h" // For GUID_All_Objects
#else
#include <objbase.h>
#include "..\shared\critsec.h"
#include "PChMap.h"
#include "dmksctrl.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "..\shared\dmstrm.h"
#include "..\shared\validate.h"
#include "dmbandp.h"
#include "bandtrk.h"
#include "debug.h"
#include "..\shared\oledll.h"
#endif
