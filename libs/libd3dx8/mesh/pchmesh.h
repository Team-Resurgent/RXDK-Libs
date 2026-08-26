/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Precompiled header for the D3DX mesh library: pulls in the D3D8, D3DX private
 * and common mesh headers that nearly every translation unit here needs.
 */

// templates generate names > 255 chars in debug info
// 4786: identifier was truncated to 'number' characters in the debug information
#pragma warning(disable : 4786)

#include <d3dx8seg.h>
#include <stdio.h>
#include <malloc.h>

#define NOD3D
#define NODSOUND
#include <xtl.h>
#undef  NOD3D
#undef NODSOUND

#include <d3d8.h>
#include "d3dx8p.h"
#include <d3d8types.h>
#include "common.h"
#include "gxcrackfvf.h"

#ifndef HeapValidate
#define HeapValidate
#endif

