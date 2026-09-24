/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Declares the MSZIP compress and decompress helpers along with the on-disk
 * block and file structures used for compressed .X files.
 */

#ifndef _XZIP_H_
#define _XZIP_H_

class UnCompressedFile
{
public:
    DWORD m_dwMagic;
    DWORD m_dwVersion;
    DWORD m_dwFormat;
    DWORD m_dwFloatSize;
    BYTE  m_pData[1];
};

class CmpBlock
{
public:
    WORD m_cbOrg;
    WORD m_cbData;
    BYTE m_pData[1];
};

class CompressedFile
{
public:
    DWORD m_dwMagic;
    DWORD m_dwVersion;
    DWORD m_dwFormat;
    DWORD m_dwFloatSize;
    DWORD m_dwOrgFileSize;
    CmpBlock m_pCmpBlock[1];
};

class ZipDecompressor
{
    LPVOID    m_nfmContext;
    MI_MEMORY m_bufDest;

public:
    ZipDecompressor();
    ~ZipDecompressor();

    BOOL Init();

    HRESULT Decompress(LPVOID pvSrc, DWORD cbSrc,
                       LPVOID *ppvDest, LPDWORD pcbDest);
};

#endif // _XZIP_H_
