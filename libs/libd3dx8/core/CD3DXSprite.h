/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declaration of CD3DXSprite, the textured-quad sprite helper.
 * See CD3DXSprite.cpp for the behaviour.
 */

#ifndef __CD3DXSprite_H__
#define __CD3DXSprite_H__


///////////////////////////////////////////////////////////////////////////
// CD3DXSprite //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

class CD3DXSprite : public ID3DXSprite
{
public:
    CD3DXSprite();
   ~CD3DXSprite();

    HRESULT Initialize(LPDIRECT3DDEVICE8 pDevice);

    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // ID3DXSprite
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE8* ppDevice);

    STDMETHOD(Begin)(THIS);

    STDMETHOD(Draw)(THIS_ LPDIRECT3DTEXTURE8  pSrcTexture, 
        CONST RECT* pSrcRect, CONST D3DXVECTOR2* pScaling, 
        CONST D3DXVECTOR2* pRotationCenter, FLOAT Rotation, 
        CONST D3DXVECTOR2* pTranslation, D3DCOLOR Color);

    STDMETHOD(DrawTransform)(THIS_ LPDIRECT3DTEXTURE8 pSrcTexture, 
        CONST RECT* pSrcRect, CONST D3DXMATRIX* pTransform, 
        D3DCOLOR Color);

    STDMETHOD(End)(THIS);

public:
    UINT                m_uRef;
    LPDIRECT3DDEVICE8   m_pDevice;
    BOOL                m_bBegin;
    DWORD               m_dwOldState;
    DWORD               m_dwNewState;
};

#endif //__CD3DXSprite_H__