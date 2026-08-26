/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Miscellaneous global declarations with no more specific home.
 */

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{

// Don't forget that 'static' for __forceinline:

#define FORCEINLINE static __forceinline

//------------------------------------------------------------------------------
// Dwordify
//
// 'Dwordify' merely changes the type of a 'float' to a 'dword', without
// having to do a float-to-int conversion, for ease of use with stuff
// like SetRenderState:

FORCEINLINE DWORD Dwordify(FLOAT f)
{
    return *((DWORD*) &f);
}

//------------------------------------------------------------------------------
// Floatify
//
// 'Dwordify' merely changes the type of a 'dword' to a 'float':

FORCEINLINE float Floatify(DWORD d)
{
    return *((float*) &d);
}

//------------------------------------------------------------------------------
// Function prototypes for Pixel and Vertex Shader Capture
//
void HandleShaderSnapshotOpcode();

void HandleShaderSnapshot_DrawVerticesUP(
    D3DPRIMITIVETYPE PrimitiveType,
    UINT VertexCount,
    CONST void* pVertexStreamZeroData,
    UINT VertexStreamZeroStride);

void HandleShaderSnapshot_DrawIndexedVerticesUP(
    D3DPRIMITIVETYPE PrimitiveType,
    UINT VertexCount,
    CONST void* pIndexData,
    CONST void* pVertexStreamZeroData,
    UINT VertexStreamZeroStride);
    
void HandleShaderSnapshot_DrawVertices(
    D3DPRIMITIVETYPE PrimitiveType,
    UINT StartVertex,
    UINT VertexCount);
    
void HandleShaderSnapshot_DrawIndexedVertices(
    D3DPRIMITIVETYPE PrimitiveType,
    UINT VertexCount,
    CONST WORD* pIndexData);

void HandleShaderSnapshot_Clear(DWORD Count,
                                CONST D3DRECT *pRects,
                                DWORD Flags,
                                D3DCOLOR Color,
                                float Z,
                                DWORD Stencil);
} // end namespace
