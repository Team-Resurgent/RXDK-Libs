/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Umbrella header for the D3DX8 utility library. Defines the common D3DX
 * conventions (D3DXINLINE, the D3DX_DEFAULT sentinels) and pulls in the module
 * headers for math, core, textures, meshes, shapes and effects, so a title need
 * only include this one header.
 */

#ifndef __D3DX8_H__
#define __D3DX8_H__

#include <d3d8.h>
#include <limits.h>

#include <xobjbase.h>

#ifndef D3DXINLINE
#ifdef __cplusplus
#define D3DXINLINE inline
#else
#define D3DXINLINE _inline
#endif
#endif

#define D3DX_DEFAULT ULONG_MAX
#define D3DX_DEFAULT_FLOAT FLT_MAX


#include "d3dx8math.h"
#include "d3dx8core.h"
#include "d3dx8tex.h"
#include "d3dx8meshp.h"
#include "d3dx8shape.h"
#include "d3dx8effect.h"

#endif //__D3DX8_H__

