/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Stand-alone surface class returned by CreateRenderTarget and used by
 * CreateZStencilSurface.
 */

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{

//------------------------------------------------------------------------------
// Creates a surface that wraps part of a texture.
//
HRESULT CreateSurfaceOfTexture(
    DWORD Format,
    DWORD Size,
    D3DBaseTexture *pParent,
    void *pvData,
    D3DSurface **ppSurface
    );

//------------------------------------------------------------------------------
// Initializes a pre-allocated surface.  This will never be a PO2 surface
// and the pitch will always match the width.  This is only used for
// frame- and back-buffers.
//
void InitializeSurface(
    D3DSurface *pSurface,
    DWORD Format,
    DWORD Size,
    void *pvData);

//------------------------------------------------------------------------------
// Creates a surface that has no owner.
//
HRESULT CreateStandAloneSurface(
    DWORD Width,
    DWORD Height, 
    D3DFORMAT D3DFormat,
    D3DSurface **ppSurface
    );

//------------------------------------------------------------------------------
// Creates a volume that wraps part of a texture.
//
HRESULT CreateVolumeOfTexture(
    DWORD Format,
    D3DBaseTexture *pParent,
    void *pvData,
    D3DVolume **ppVolume
    );

//------------------------------------------------------------------------------
// Creates a surface in which the header and data are contiguous.
//
HRESULT CreateSurfaceWithContiguousHeader(
    DWORD Width,
    DWORD Height,     
    D3DFORMAT D3DFormat,
    D3DSurface **ppSurface
    );

} // end namespace
