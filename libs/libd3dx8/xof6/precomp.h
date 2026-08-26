/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Precompiled header for the xof6 (DirectX .X file) parser. Pulls in the D3DX
 * segment, CRT and XTL headers plus every internal xof6 class header so each
 * translation unit shares one consistent include set.
 */

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <d3dx8seg.h>

#include <stddef.h>
#include <stdio.h>
#include <xtl.h>

#include "memalloc.h"
#include "dpf.h"

#include "dxfile.h"

#include "xheader.h"
#include "xmemory.h"
#include "xparse.h"

#include "xlist.h"

#include "xprim.h"
#include "xstring.h"

#include "xobject.h"
#include "xtempl.h"
#include "xdata.h"
#include "xblob.h"

#include "xstrmrd.h"
#include "ximplapi.h"


#endif // _PRECOMP_H_
