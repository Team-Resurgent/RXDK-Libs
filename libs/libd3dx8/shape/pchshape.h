/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Precompiled header for the D3DX shape library: the common D3DX and Xbox
 * includes shared by the shape-generation sources, plus the RELEASE helper macro.
 */

#include <d3dx8seg.h>
#include <xtl.h>
#include "d3dx8dbg.h"
#include "malloc.h"

#define RELEASE(x) \
    do { if(x) { x->Release(); x = NULL; } } while(0)
