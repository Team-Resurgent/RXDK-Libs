/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Precompiled header for the D3DX8 effect module: pulls in the CRT, xtl.h and
 * the D3DX8/D3D8 public headers, forward-declares the effect classes, includes
 * the compiler/declaration/effect/technique headers, and defines the RELEASE()
 * convenience macro used throughout.
 */

#ifndef __PCHEFFECT_H__
#define __PCHEFFECT_H__

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

class CEffectNode;
class CD3DXValue;
class CD3DXParameter;
class CD3DXAssignment;
class CD3DXPass;
class CD3DXTechnique;
class CD3DXEffect;

#include "CD3DXStack.h"
#include "CCompiler.h"
#include "CDeclaration.h"
#include "CEffect.h"
#include "CTechnique.h"

#define RELEASE(x) \
    do { if(x) { x->Release(); x = NULL; } } while(0)


#endif //__PCHEFFECT_H__//
