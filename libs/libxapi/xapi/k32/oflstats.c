/*++

Copyright (c) Microsoft Corporation

Module Name:

    oflstats.c

Abstract:

    Offline statistics store on T: drive (StatsXBE\UserData.sdb).

--*/

#include "basedll.h"
#pragma hdrstop

#include <xcrypt.h>

#define OFLSTAT_MAGIC           ((DWORD)'1SFO')   // 'OFS1'
#define OFLSTAT_HDR_SIZE        0x1C
#define OFLSTAT_HDR_TOTAL       0x24
#define OFLSTAT_RECORD_SIZE     0x62
#define OFLSTAT_USERNAME_MAX    31

static const CHAR g_szStatDir[]  = "t:\\StatsXBE";
static const CHAR g_szStatFile[] = "t:\\StatsXBE\\UserData.sdb";

HANDLE g_hOflStatFile = INVALID_HANDLE_VALUE;

typedef struct _OFLSTAT_HEADER {
    DWORD dwMagic;
    DWORD dwHeaderSize;
    DWORD dwReserved;
    DWORD dwRecordCount;
    BYTE  rgbSignature[20];
} OFLSTAT_HEADER, *POFLSTAT_HEADER;

typedef struct _OFLSTAT_RECORD {
    BYTE rgbBody[OFLSTAT_RECORD_SIZE - 20];
    BYTE rgbSignature[20];
} OFLSTAT_RECORD, *POFLSTAT_RECORD;

#define OFLSTAT_OFF_USERNAME            0
#define OFLSTAT_OFF_LEADERBOARD         0x40
#define OFLSTAT_OFF_ATTRIBUTE           0x44
#define OFLSTAT_OFF_VALUE               0x46

static
DWORD
XapipStatCalculateHeaderSignature(
    POFLSTAT_HEADER pHeader
    )
{
    DWORD dwErr;
    HANDLE hSig;

    memset(pHeader->rgbSignature, 0, sizeof(pHeader->rgbSignature));
    pHeader->dwMagic = OFLSTAT_MAGIC;
    pHeader->dwHeaderSize = OFLSTAT_HDR_SIZE;

    hSig = XCalculateSignatureBegin(1);
    if (hSig == (HANDLE)-1) {
        return GetLastError();
    }

    dwErr = XCalculateSignatureUpdate(hSig, (PBYTE)pHeader, 0x10);
    if (dwErr == 0) {
        dwErr = XCalculateSignatureEnd(hSig, pHeader->rgbSignature);
    }

    if (hSig != (HANDLE)-1) {
        XCalculateSignatureEnd(hSig, NULL);
    }

    return dwErr;
}

static
DWORD
XapipStatCalculateRecordSignature(
    POFLSTAT_RECORD pRecord
    )
{
    DWORD dwErr;
    HANDLE hSig;

    memset(pRecord->rgbSignature, 0, sizeof(pRecord->rgbSignature));

    hSig = XCalculateSignatureBegin(1);
    if (hSig == (HANDLE)-1) {
        return GetLastError();
    }

    dwErr = XCalculateSignatureUpdate(hSig, (PBYTE)pRecord, 0x4E);
    if (dwErr == 0) {
        dwErr = XCalculateSignatureEnd(hSig, pRecord->rgbSignature);
    }

    if (hSig != (HANDLE)-1) {
        XCalculateSignatureEnd(hSig, NULL);
    }

    return dwErr;
}

static
BOOL
XapipStatIsHeaderSignatureValid(
    POFLSTAT_HEADER pHeader
    )
{
    BYTE rgbComputed[20];
    HANDLE hSig;
    BOOL fValid = FALSE;

    hSig = XCalculateSignatureBegin(1);
    if (hSig == (HANDLE)-1) {
        return FALSE;
    }

    if (XCalculateSignatureUpdate(hSig, (PBYTE)pHeader, 0x10) == 0 &&
        XCalculateSignatureEnd(hSig, rgbComputed) == 0 &&
        memcmp(rgbComputed, pHeader->rgbSignature, sizeof(rgbComputed)) == 0)
    {
        fValid = TRUE;
    }

    if (hSig != (HANDLE)-1) {
        XCalculateSignatureEnd(hSig, NULL);
    }

    return fValid;
}

static
BOOL
XapipStatIsRecordSignatureValid(
    POFLSTAT_RECORD pRecord
    )
{
    BYTE rgbComputed[20];
    HANDLE hSig;
    BOOL fValid = FALSE;

    hSig = XCalculateSignatureBegin(1);
    if (hSig == (HANDLE)-1) {
        return FALSE;
    }

    if (XCalculateSignatureUpdate(hSig, (PBYTE)pRecord, 0x4E) == 0 &&
        XCalculateSignatureEnd(hSig, rgbComputed) == 0 &&
        memcmp(rgbComputed, pRecord->rgbSignature, sizeof(rgbComputed)) == 0)
    {
        fValid = TRUE;
    }

    if (hSig != (HANDLE)-1) {
        XCalculateSignatureEnd(hSig, NULL);
    }

    return fValid;
}

static
BOOL
XapipStatReadStatHeader(
    HANDLE hFile,
    POFLSTAT_HEADER pHeader
    )
{
    DWORD cbRead;

    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == (DWORD)-1) {
        return FALSE;
    }

    if (!ReadFile(hFile, &pHeader->dwMagic, sizeof(DWORD), &cbRead, NULL) ||
        cbRead != sizeof(DWORD) ||
        pHeader->dwMagic != OFLSTAT_MAGIC)
    {
        return FALSE;
    }

    if (!ReadFile(hFile, &pHeader->dwHeaderSize, sizeof(DWORD), &cbRead, NULL) ||
        cbRead != sizeof(DWORD) ||
        pHeader->dwHeaderSize != OFLSTAT_HDR_SIZE)
    {
        return FALSE;
    }

    if (!ReadFile(hFile, &pHeader->dwReserved, OFLSTAT_HDR_TOTAL - 2 * sizeof(DWORD),
                   &cbRead, NULL) ||
        cbRead != OFLSTAT_HDR_TOTAL - 2 * sizeof(DWORD))
    {
        return FALSE;
    }

    return XapipStatIsHeaderSignatureValid(pHeader);
}

static
DWORD
XapipStatWriteStatHeader(
    HANDLE hFile,
    POFLSTAT_HEADER pHeader
    )
{
    DWORD cbWritten;

    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == (DWORD)-1) {
        return GetLastError();
    }

    if (!WriteFile(hFile, pHeader, OFLSTAT_HDR_TOTAL, &cbWritten, NULL)) {
        return GetLastError();
    }

    return 0;
}

static
DWORD
XapipStatCalculateSignatureAndWriteHeader(
    HANDLE hFile,
    POFLSTAT_HEADER pHeader
    )
{
    DWORD dwErr = XapipStatCalculateHeaderSignature(pHeader);
    if (dwErr != 0) {
        return dwErr;
    }
    return XapipStatWriteStatHeader(hFile, pHeader);
}

static
DWORD
XapipStatDBCreate(
    LPCSTR pszFile,
    LPCSTR pszDir,
    DWORD cbFileSize,
    DWORD dwOpenMode
    )
{
    OFLSTAT_HEADER Header;
    DWORD dwRecordCount;
    DWORD dwErr = 0;

    CreateDirectoryA(pszDir, NULL);

    if (g_hOflStatFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hOflStatFile);
        g_hOflStatFile = INVALID_HANDLE_VALUE;
    }

    g_hOflStatFile = CreateFileA(
        pszFile,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        dwOpenMode,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (g_hOflStatFile == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    if (SetFilePointer(g_hOflStatFile, cbFileSize, NULL, FILE_BEGIN) == (DWORD)-1 ||
        !SetEndOfFile(g_hOflStatFile))
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    dwRecordCount = (cbFileSize - OFLSTAT_HDR_TOTAL) / OFLSTAT_RECORD_SIZE;

    RtlZeroMemory(&Header, sizeof(Header));
    Header.dwMagic = OFLSTAT_MAGIC;
    Header.dwHeaderSize = OFLSTAT_HDR_SIZE;
    Header.dwReserved = 0;
    Header.dwRecordCount = dwRecordCount;

    dwErr = XapipStatCalculateSignatureAndWriteHeader(g_hOflStatFile, &Header);

Cleanup:
    if (dwErr != 0) {
        CloseHandle(g_hOflStatFile);
        g_hOflStatFile = INVALID_HANDLE_VALUE;
        DeleteFileA(pszFile);
        RemoveDirectoryA(pszDir);
    }

    return dwErr;
}

static
DWORD
XapipStatValidateAndCreate(
    LPCSTR pszFile,
    LPCSTR pszDir,
    DWORD cbMaxSize,
    DWORD dwOpenMode
    )
{
    DWORD cbFileSize;

    if (cbMaxSize == 0) {
        if (g_hOflStatFile != INVALID_HANDLE_VALUE) {
            CloseHandle(g_hOflStatFile);
            g_hOflStatFile = INVALID_HANDLE_VALUE;
        }
        DeleteFileA(pszFile);
        RemoveDirectoryA(pszDir);
        return 0;
    }

    if (cbMaxSize < 0x40000) {
        cbMaxSize = 0x40000;
    } else if (cbMaxSize > 0x400000) {
        cbMaxSize = 0x400000;
    }

    if (dwOpenMode != 1 && dwOpenMode != 2) {
        return ERROR_INVALID_PARAMETER;
    }

    cbFileSize = ((cbMaxSize - OFLSTAT_HDR_TOTAL) / OFLSTAT_RECORD_SIZE) * OFLSTAT_RECORD_SIZE
                 + OFLSTAT_HDR_TOTAL;

    return XapipStatDBCreate(pszFile, pszDir, cbFileSize, dwOpenMode);
}

static
DWORD
XapipStatWrite(
    LPCSTR pszFile,
    LPCSTR pszDir,
    LPCWSTR lpLocalUserName,
    DWORD dwLeaderBoardIndex,
    WORD wAttributeIndex,
    XOFFLINE_STAT_TYPE Type,
    VOID* pStatValue
    )
{
    OFLSTAT_HEADER Header;
    OFLSTAT_RECORD Record;
    DWORD cbWritten;
    DWORD dwErr = 0;
    BOOL fHeaderUpdated = FALSE;

    if (lpLocalUserName == NULL || lpLocalUserName[0] == L'\0') {
        return ERROR_INVALID_PARAMETER;
    }

    if (g_hOflStatFile == INVALID_HANDLE_VALUE) {
        g_hOflStatFile = CreateFileA(
            pszFile,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (g_hOflStatFile == INVALID_HANDLE_VALUE) {
            dwErr = XapipStatDBCreate(pszFile, pszDir, 0x200000, OPEN_ALWAYS);
            if (dwErr != 0) {
                return dwErr;
            }
        }
    }

    if (!XapipStatReadStatHeader(g_hOflStatFile, &Header)) {
        return ERROR_FILE_INVALID;
    }

    if (Header.dwRecordCount >= Header.dwReserved) {
        return 0x10DA;
    }

    if (SetFilePointer(g_hOflStatFile,
                       Header.dwRecordCount * OFLSTAT_RECORD_SIZE + OFLSTAT_HDR_TOTAL,
                       NULL,
                       FILE_BEGIN) == (DWORD)-1)
    {
        return GetLastError();
    }

    RtlZeroMemory(&Record, sizeof(Record));
    wcsncpy((PWCHAR)&Record.rgbBody[OFLSTAT_OFF_USERNAME],
            lpLocalUserName,
            OFLSTAT_USERNAME_MAX);
    *(PDWORD)&Record.rgbBody[OFLSTAT_OFF_LEADERBOARD] = dwLeaderBoardIndex;
    *(PWORD)&Record.rgbBody[OFLSTAT_OFF_ATTRIBUTE] = wAttributeIndex;

    switch (Type) {
    case XOFFLINE_STAT_LONG:
        *(PDWORD)&Record.rgbBody[OFLSTAT_OFF_VALUE] = *(PDWORD)pStatValue;
        break;
    case XOFFLINE_STAT_LONGLONG:
        memcpy(&Record.rgbBody[OFLSTAT_OFF_VALUE], pStatValue, sizeof(LONGLONG));
        break;
    case XOFFLINE_STAT_DOUBLE:
        *(double UNALIGNED *)&Record.rgbBody[OFLSTAT_OFF_VALUE] = *(double *)pStatValue;
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = XapipStatCalculateRecordSignature(&Record);
    if (dwErr != 0) {
        goto Cleanup;
    }

    if (!WriteFile(g_hOflStatFile, &Record, OFLSTAT_RECORD_SIZE, &cbWritten, NULL)) {
        dwErr = GetLastError();
        goto Cleanup;
    }

    Header.dwRecordCount++;
    dwErr = XapipStatCalculateSignatureAndWriteHeader(g_hOflStatFile, &Header);
    fHeaderUpdated = TRUE;

Cleanup:
    if (dwErr != 0 && fHeaderUpdated) {
        CloseHandle(g_hOflStatFile);
        g_hOflStatFile = INVALID_HANDLE_VALUE;
        DeleteFileA(pszFile);
        RemoveDirectoryA(pszDir);
    }

    return dwErr;
}

DWORD
__attribute__((__stdcall__))
XCreateStatStore(
    DWORD cbMaxSize,
    DWORD dwOpenMode
    )
{
    return XapipStatValidateAndCreate(g_szStatFile, g_szStatDir, cbMaxSize, dwOpenMode);
}

DWORD
__attribute__((__stdcall__))
XWriteStatStore(
    LPCWSTR lpLocalUserName,
    DWORD dwLeaderBoardIndex,
    WORD wAttributeIndex,
    XOFFLINE_STAT_TYPE Type,
    VOID* pStatValue
    )
{
    if (Type != XOFFLINE_STAT_LONG &&
        Type != XOFFLINE_STAT_LONGLONG &&
        Type != XOFFLINE_STAT_DOUBLE)
    {
        return ERROR_INVALID_PARAMETER;
    }

    return XapipStatWrite(
        g_szStatFile,
        g_szStatDir,
        lpLocalUserName,
        dwLeaderBoardIndex,
        wAttributeIndex,
        Type,
        pStatValue);
}

DWORD
__attribute__((__stdcall__))
XClearStatStore(
    LPCWSTR lpLocalUserName
    )
{
    OFLSTAT_HEADER Header;
    OFLSTAT_RECORD Record;
    DWORD dwRecordTotal;
    DWORD dwIndex;
    DWORD dwErr = 0;
    DWORD cbRead;

    if (lpLocalUserName == NULL) {
        return XCreateStatStore(0, 1);
    }

    if (g_hOflStatFile == INVALID_HANDLE_VALUE) {
        g_hOflStatFile = CreateFileA(
            g_szStatFile,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (g_hOflStatFile == INVALID_HANDLE_VALUE) {
            return GetLastError();
        }
    }

    if (!XapipStatReadStatHeader(g_hOflStatFile, &Header)) {
        CloseHandle(g_hOflStatFile);
        g_hOflStatFile = INVALID_HANDLE_VALUE;
        DeleteFileA(g_szStatFile);
        RemoveDirectoryA(g_szStatDir);
        return ERROR_FILE_INVALID;
    }

    dwRecordTotal = Header.dwRecordCount;
    Header.dwRecordCount = 0;

    for (dwIndex = 0; dwIndex < dwRecordTotal; dwIndex++) {
        if (SetFilePointer(g_hOflStatFile,
                           dwIndex * OFLSTAT_RECORD_SIZE + OFLSTAT_HDR_TOTAL,
                           NULL,
                           FILE_BEGIN) == (DWORD)-1)
        {
            dwErr = GetLastError();
            break;
        }

        if (!ReadFile(g_hOflStatFile, &Record, OFLSTAT_RECORD_SIZE, &cbRead, NULL) ||
            cbRead != OFLSTAT_RECORD_SIZE)
        {
            dwErr = GetLastError();
            break;
        }

        if (wcscmp((PWCHAR)&Record.rgbBody[OFLSTAT_OFF_USERNAME], lpLocalUserName) == 0) {
            continue;
        }

        if (SetFilePointer(g_hOflStatFile,
                           Header.dwRecordCount * OFLSTAT_RECORD_SIZE + OFLSTAT_HDR_TOTAL,
                           NULL,
                           FILE_BEGIN) == (DWORD)-1)
        {
            dwErr = GetLastError();
            break;
        }

        if (!WriteFile(g_hOflStatFile, &Record, OFLSTAT_RECORD_SIZE, &cbRead, NULL)) {
            dwErr = GetLastError();
            break;
        }

        Header.dwRecordCount++;
    }

    if (dwErr == 0) {
        dwErr = XapipStatCalculateSignatureAndWriteHeader(g_hOflStatFile, &Header);
    } else {
        dwErr = GetLastError();
    }

    return dwErr;
}
