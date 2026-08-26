/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Precompiled header for the D3DX8 core module: pulls in the CRT, xtl.h and the
 * D3DX8/D3D8 public headers, then the core helper class declarations (files,
 * stacks, buffers, sprites, render-to-surface and render-to-env-map), and
 * defines the RELEASE() convenience macro used throughout.
 */

#ifndef __PCHCORE_H__
#define __PCHCORE_H__
#include <d3dx8seg.h>

#define D3DCOMPILE_BEGINSTATEBLOCK 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <xtl.h>

#include "d3dx8.h"
#include "d3dx8dbg.h"
#include "d3d8types.h"

#include "CD3DXFile.h"
#include "CD3DXStack.h"
#include "CD3DXBuffer.h"
#include "CD3DXSprite.h"
#include "CD3DXRenderToSurface.h"
#include "CD3DXRenderToEnvMap.h"

#define RELEASE(x) \
    do { if(x) { x->Release(); x = NULL; } } while(0)

#endif //__PCHCORE_H__