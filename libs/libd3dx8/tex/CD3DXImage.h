/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declaration of the CD3DXImage class - the decoded-image container used by the
 * texture loaders. Holds the pixel data along with its format, pitch, rectangle
 * and optional palette. Implemented in CD3DXImage.cpp.
 */

#ifndef __CD3DXImage_H__
#define __CD3DXImage_H__


///////////////////////////////////////////////////////////////////////////
// CD3DXImage /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

class CD3DXImage
{
public:
    D3DFORMAT       m_Format;
    LPVOID          m_pvData;
    DWORD           m_cbPitch;
    RECT            m_Rect;
    PALETTEENTRY*   m_pPalette;

    BOOL            m_bDeleteData;
    BOOL            m_bDeletePalette;

    CD3DXImage*     m_pMip;
    CD3DXImage*     m_pFace;


public:
    CD3DXImage();
    ~CD3DXImage();

    HRESULT Load(LPCVOID pvData, DWORD cbData, D3DXIMAGE_INFO *pInfo);

private:
    HRESULT LoadBMP(LPCVOID pvData, DWORD cbData);
    HRESULT LoadDIB(LPCVOID pvData, DWORD cbData);
    HRESULT LoadJPG(LPCVOID pvData, DWORD cbData);
    HRESULT LoadTGA(LPCVOID pvData, DWORD cbData);
    HRESULT LoadPPM(LPCVOID pvData, DWORD cbData);
    HRESULT LoadDDS(LPCVOID pvData, DWORD cbData);
    HRESULT LoadPNG(LPCVOID pvData, DWORD cbData);
};


#endif