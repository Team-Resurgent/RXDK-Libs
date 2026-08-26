/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declarations for the D3DX8 buffer objects: CD3DXBuffer (the ID3DXBuffer
 * implementation) and CD3DXStringBuffer (its two-ended string-arena subclass),
 * plus the D3DXCreateStringBuffer helper. See CD3DXBuffer.cpp for details.
 */

#ifndef __D3DXBUFFER_H__
#define __D3DXBUFFER_H__

class CD3DXBuffer : public ID3DXBuffer
{
public:
    CD3DXBuffer();
   ~CD3DXBuffer();

    HRESULT Init(DWORD dwSize);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // ID3DXBuffer
    virtual PVOID STDMETHODCALLTYPE GetBufferPointer();
    virtual DWORD STDMETHODCALLTYPE GetBufferSize();

protected:
    ULONG m_cRef;
    DWORD m_dwSize;
    PBYTE m_pbBuffer;
    HRESULT Grow(DWORD dwSize);
};

class CD3DXStringBuffer : public CD3DXBuffer
{
private:
    char** m_pStringLow;
    char* m_pStringHigh;
public:
    CD3DXStringBuffer();
    HRESULT Init(DWORD size);
    HRESULT AddString(char* pString);
};

HRESULT D3DXCreateStringBuffer(DWORD size, CD3DXStringBuffer** ppBuffer);
#endif