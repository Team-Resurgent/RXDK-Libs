/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * D3DX utility library umbrella header. Pulls in the full D3DX8 helper surface
 * (math, core objects, texture/surface loaders, mesh, shape, effect) in one
 * include. D3DX layers convenience routines on top of the low-level Xbox
 * Direct3D 8 API in d3d8.h; titles include this to get all of them at once.
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

/* Sentinel "pick a sensible default" values passed to D3DX helpers in place of
 * an explicit parameter (e.g. mip-level count, filter, size). D3DX_DEFAULT is
 * the integer form (ULONG_MAX); D3DX_DEFAULT_FLOAT the float form (FLT_MAX). */
#define D3DX_DEFAULT ULONG_MAX
#define D3DX_DEFAULT_FLOAT FLT_MAX

#include "d3dx8math.h"
#include "d3dx8core.h"
#include "d3dx8tex.h"
#include "d3dx8mesh.h"
#include "d3dx8shape.h"
#include "d3dx8effect.h"


#endif //__D3DX8_H__
