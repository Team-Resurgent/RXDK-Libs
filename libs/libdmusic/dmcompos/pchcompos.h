/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Precompiled-header aggregate for the composer component: pulls in the composer
 * classes, DirectMusic interfaces, the memory-stream helper and the shared
 * critical-section, validation and scripting-GUID headers every translation
 * unit in this slice needs.
 */

#include "DMCompos.h"
#include "debug.h"
#include "DMPers.h"
#ifdef XBOX
#include <xtl.h>
#else
#include <objbase.h>
#endif
#include <ole2.h>
#include "memstm.h"
#include "..\shared\critsec.h"
#include "..\shared\validate.h"
#include "..\shared\dmscriptautguids.h"
#include <time.h>   // to seed random number generator
#include "debug.h"
#include "..\shared\oledll.h"
#include "dmusicip.h"
#include "DMCompP.h"
#include "dmpers.h"
#include "dmcompos.h"
#include "dmtempl.h"
#include "spsttrk.h"
#include "perstrk.h"
#include "..\dmstyle\iostru.h"
#include "..\dmime\dmgraph.h"
#include "..\shared\xcreate.h"
#include "..\shared\Validate.h"
#include "..\shared\miscutil.h"
#include "..\dmstyle\dmstyleP.h"
#include "..\dmime\dmgraph.h"
#include "dmusicf.h"
#include "str.h"

#ifdef XBOX
#define CreateStreamOnHGlobal DMusic_CreateStreamOnHGlobal
STDAPI DMusic_CreateStreamOnHGlobal(HANDLE hGlobal, BOOL fDeleteOnRelease,
        LPSTREAM FAR* ppstm);
#endif
