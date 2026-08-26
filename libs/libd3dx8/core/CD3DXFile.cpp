/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * CD3DXFile -- a simple whole-file reader. Open() takes an ANSI or Unicode
 * path, opens the file for sequential read, allocates a buffer of the file's
 * size and slurps the entire contents into memory; the data and its byte count
 * are exposed as public members. Close() (also run by the destructor) frees
 * the buffer. Used by the loaders that want a file mapped in one shot.
 */

#include "pchcore.h"


///////////////////////////////////////////////////////////////////////////
// CD3DXFile //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


CD3DXFile::CD3DXFile()
{
    m_pvData   = NULL;
    m_cbData   = 0;
}


CD3DXFile::~CD3DXFile()
{
    Close();
}


HRESULT CD3DXFile::Open(LPCVOID pFile, BOOL bUnicode)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;

    if(!pFile)
        return E_INVALIDARG;

    char *szFile, szFileBuf[_MAX_PATH];

    if (bUnicode)
    {
        WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pFile, -1, szFileBuf, _MAX_PATH, NULL, NULL);
        szFile = szFileBuf;
    }
    else
    {
        szFile = (char *)pFile;
    }

    hFile = CreateFileA(szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if(INVALID_HANDLE_VALUE == hFile)
        goto LError;

    // Get file size
    m_cbData = GetFileSize(hFile, NULL);

    if(0xffffffff == m_cbData)
        goto LError;

    // Make room.
    m_pvData = (const void *)malloc(m_cbData);

    if (!m_pvData)
    {
        goto LError;
    }

    // Read it in.
    DWORD dwRead;

    if (!ReadFile(hFile, (void *)m_pvData, m_cbData, &dwRead, NULL))
    {
        goto LError;
    }

    CloseHandle(hFile);

    return S_OK;

LError:
    DWORD dwError;
    dwError = GetLastError();

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT CD3DXFile::Close()
{
    if(m_pvData)
    {
        free((void *)m_pvData);

        m_pvData = NULL;
        m_cbData = 0;
    }

    return S_OK;
}
