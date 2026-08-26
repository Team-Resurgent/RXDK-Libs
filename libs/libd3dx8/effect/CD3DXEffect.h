/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declarations of EffectData (the parsed effect representation) and CD3DXEffect
 * (the ID3DXEffect implementation). See CD3DXEffect.cpp for the behaviour.
 */

#ifndef __CD3DXEFFECT_H__
#define __CD3DXEFFECT_H__

class EffectData
{
public:
    UINT m_uDwords;
    UINT m_uFloats;
    UINT m_uColors;
    UINT m_uVectors;
    UINT m_uMatrices;
    UINT m_uTextures;
    UINT m_uTechniques;

    UINT m_uMinLOD;
    UINT m_uMaxLOD;

    TechniqueData *m_pTechnique;

public:
    EffectData();
   ~EffectData();
};


//////////////////////////////////////////////////////////////////////////////
// CD3DXEffect ///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class CD3DXEffect : public ID3DXEffect
{
public:
    CD3DXEffect();
   ~CD3DXEffect();

    HRESULT Initialize(IDirect3DDevice8 *pDevice, EffectData *pData);


    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // ID3DXEffect
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice8 **ppDevice);
    STDMETHOD_(D3DXEFFECT_DESC, GetEffectDesc)(THIS);

    STDMETHOD(SetDword)  (THIS_ UINT uIndex, DWORD dw);
    STDMETHOD(SetFloat)  (THIS_ UINT uIndex, float f);
    STDMETHOD(SetColor)  (THIS_ UINT uIndex, D3DCOLOR Color);
    STDMETHOD(SetVector) (THIS_ UINT uIndex, D3DXVECTOR4 *pVector);
    STDMETHOD(SetMatrix) (THIS_ UINT uIndex, D3DMATRIX *pMatrix);
    STDMETHOD(SetTexture)(THIS_ UINT uIndex, IDirect3DBaseTexture8 *pTexture);

    STDMETHOD(GetTechnique) (THIS_ UINT uIndex, ID3DXTechnique **ppTechnique);
    STDMETHOD(PickTechnique)(THIS_ UINT uMinLOD, UINT uMaxLOD, ID3DXTechnique **ppTechnique);


public:
    UINT m_uRefCount;
    EffectData *m_pData;
    IDirect3DBaseTexture8 **m_ppSurface;
    D3DXMATRIX *m_pMatrix;
    IDirect3DDevice8 *m_pDevice;
};

#endif //__CD3DXEFFECT_H__

