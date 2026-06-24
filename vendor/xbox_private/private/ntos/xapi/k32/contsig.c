/*++

Copyright (c) Microsoft Corporation

Description:
    Content signature installation, verification, and lookup.

Module Name:

    contsig.c

--*/

#include "basedll.h"
#pragma hdrstop

#include <xboxp.h>
#include <sha.h>
#include <shahmac.h>

#ifdef DeleteFile
#undef DeleteFile
#endif

//
// Exported metadata string constants (also referenced by findcont.c via xmeta.h
// duplicates, but required as public symbols from this object).
//

const OCHAR g_cszContentMetaFileName[] = OTEXT("\\ContentMeta.xbx");
const int   g_cchContentMetaFileName   = ARRAYSIZE(g_cszContentMetaFileName) - 1;

const WCHAR g_cszNameTag[] = L"Name";
const WCHAR g_cszTitleNameTag[] = L"TitleName";

const OCHAR g_cszStar[] = OTEXT("*");
const int   g_cchStar   = ARRAYSIZE(g_cszStar) - 1;

const OCHAR g_cszContentSearch[] = OTEXT("$C\\*");
const int   g_cchContentSearch   = ARRAYSIZE(g_cszContentSearch) - 1;

const OCHAR g_cszContentDir[] = OTEXT("$C\\");
const int   g_cchContentDir   = ARRAYSIZE(g_cszContentDir) - 1;

const WCHAR g_cszDefaultLanguage[] = L"[Default]";
const WCHAR g_cszCRLF[] = L"\r\n";
const WCHAR g_cszNoCopyTrue[] = L"NoCopy=1\r\n";
const WCHAR g_chEqual = L'=';
const WCHAR g_chUnicodeSignature = 0xfeff;

const OCHAR XapipContentDirectoryTemplate[] = OTEXT("T:\\$C\\%08X%08X");

COBJECT_STRING WDrive = CONSTANT_OBJECT_STRING(OTEXT("\\??\\W:"));

#define XCONTENT_METADATA_MAGIC             0x46534358  // 'XCSF'
#define XCONTENT_METADATA_HEADER_SIZE       0x88
#define XCONTENT_SIGNATURE_HANDLE_MAGIC     0x66736378  // 'xcsf'
#define XCONTENT_SIGNATURE_SIZE             0x14
#define XCONTENT_OPTIONAL_SECTION_SIZE      0x70
#define XCONTENT_OPTIONAL_SECTION_TAG       0x0070
#define XCONTENT_OPTIONAL_SECTION_VERSION   0x0001
#define XCONTENT_SIGNATURE_FILE_TAG         0x0070
#define XCONTENT_SIGNATURE_FILE_VERSION     0x0001
#define XCONTENT_INSTALL_COPY_CHUNK         0x4000
#define XMEMALLOC_XAPI_ATTRIBUTES           0x24830000

typedef struct _XCONTENT_METADATA_HEADER
{
    BYTE rgbHeaderSignature[XCONTENT_SIGNATURE_SIZE];
    DWORD dwMagic;
    DWORD cbHeader;
    DWORD dwVersion;
    DWORD dwFlags;
    DWORD dwTitleID;
    DWORD dwOfferingIdHigh;
    DWORD dwOfferingIdLow;
    DWORD dwContentFlags;
    DWORD dwUnicodeOffset;
    DWORD cbUnicode;
    BYTE rgbUnicodeDigest[XCONTENT_SIGNATURE_SIZE];
    BYTE rgbReserved[0x30];
    DWORD dwOptionalSectionOffset;
    DWORD cbOptionalSection;
} XCONTENT_METADATA_HEADER, *PXCONTENT_METADATA_HEADER;

C_ASSERT(sizeof(XCONTENT_METADATA_HEADER) == XCONTENT_METADATA_HEADER_SIZE);

typedef struct _XCONTENT_SIGNATURE_FILE_HEADER
{
    USHORT wTag;
    USHORT wVersion;
    USHORT cEntries;
    USHORT wNameLength;
    DWORD dwStringTableOffset;
    BYTE rgbTree[0x60];
} XCONTENT_SIGNATURE_FILE_HEADER, *PXCONTENT_SIGNATURE_FILE_HEADER;

typedef struct _XCONTENT_SIGNATURE_INDEX
{
    DWORD dwDataOffset;
    DWORD dwDataSize;
} XCONTENT_SIGNATURE_INDEX, *PXCONTENT_SIGNATURE_INDEX;

typedef struct _XCONTENT_SIGNATURES
{
    DWORD dwMagic;
    PBYTE pbTreeEnd;
    PBYTE pbDataEnd;
    PBYTE pbSection;
    PVOID pvOptionalLive;
} XCONTENT_SIGNATURES, *PXCONTENT_SIGNATURES;

VOID
WINAPI
XapiComputeContentMetadataFileName(
    IN PCSTR pszDirectory,
    OUT PSTR pszMetadataFileName
    )
{
    PSTR pszEnd;
    int cchDir;

    pszEnd = pszMetadataFileName;
    while (*pszDirectory)
    {
        *pszEnd++ = *pszDirectory++;
    }

    cchDir = (int)(pszEnd - pszMetadataFileName);

    {
        PCSTR pszMetaSuffix = g_cszContentMetaFileName;

        while (*pszMetaSuffix)
        {
            pszMetadataFileName[cchDir++] = *pszMetaSuffix++;
        }
    }

    pszMetadataFileName[cchDir] = '\0';
}

VOID
WINAPI
XComputeContentSignatureKey(
    IN DWORD dwTitleId,
    OUT PBYTE pbKey
    )
{
    XcHMAC((PUCHAR)XboxHDKey,
           XBOX_KEY_LENGTH,
           (PUCHAR)&dwTitleId,
           sizeof(dwTitleId),
           NULL,
           0,
           pbKey);
}

BOOL
WINAPI
XapiComputeContentHeaderSignature(
    IN HANDLE hFile,
    IN PXCONTENT_METADATA_HEADER pHeader,
    IN DWORD dwTitleId,
    OUT PBYTE pbSignature
    )
{
    XSHAHMAC_CONTEXT HmacContext;
    BYTE rgbKey[XBOX_KEY_LENGTH];
    DWORD cbHeaderPortion;
    DWORD cbRemaining;
    DWORD cbRead;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER ByteOffset;
    BYTE rgbBuffer[512];
    NTSTATUS Status;

    if (0 == dwTitleId)
    {
        dwTitleId = pHeader->dwTitleID;
    }

    XComputeContentSignatureKey(dwTitleId, rgbKey);

    cbHeaderPortion = pHeader->cbHeader;
    if (cbHeaderPortion > XCONTENT_METADATA_HEADER_SIZE)
    {
        cbHeaderPortion = XCONTENT_METADATA_HEADER_SIZE;
    }

    cbHeaderPortion -= FIELD_OFFSET(XCONTENT_METADATA_HEADER, dwMagic);

    XShaHmacInitialize(rgbKey, XBOX_KEY_LENGTH, HmacContext);
    XShaHmacUpdate(HmacContext,
                   (PBYTE)&pHeader->dwMagic,
                   cbHeaderPortion);

    if ((NULL != hFile) && (pHeader->cbHeader > XCONTENT_METADATA_HEADER_SIZE))
    {
        cbRemaining = pHeader->cbHeader - XCONTENT_METADATA_HEADER_SIZE;
        ByteOffset.QuadPart = XCONTENT_METADATA_HEADER_SIZE;

        while (cbRemaining)
        {
            DWORD cbChunk = min(cbRemaining, sizeof(rgbBuffer));

            Status = NtReadFile(hFile,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatus,
                                rgbBuffer,
                                cbChunk,
                                &ByteOffset);

            if (!NT_SUCCESS(Status))
            {
                XapiSetLastNTError(Status);
                return FALSE;
            }

            if ((DWORD)IoStatus.Information != cbChunk)
            {
                XapiSetLastNTError(STATUS_INVALID_IMAGE_FORMAT);
                return FALSE;
            }

            XShaHmacUpdate(HmacContext, rgbBuffer, cbChunk);
            cbRemaining -= cbChunk;
            ByteOffset.QuadPart += cbChunk;
        }
    }

    XShaHmacComputeFinal(HmacContext, rgbKey, XBOX_KEY_LENGTH, pbSignature);
    return TRUE;
}

BOOL
WINAPI
XapiLoadContentMetadataHeader(
    IN HANDLE hFile,
    IN BOOL fVerifySignature,
    IN DWORD dwTitleId,
    OUT PVOID pvHeaderOptional
    )
{
    PXCONTENT_METADATA_HEADER pHeader;
    XCONTENT_METADATA_HEADER LocalHeader;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER ByteOffset;
    NTSTATUS Status;
    DWORD cbRead;
    BYTE rgbComputed[XCONTENT_SIGNATURE_SIZE];

    pHeader = (PXCONTENT_METADATA_HEADER)(pvHeaderOptional ? pvHeaderOptional : &LocalHeader);

    Status = NtReadFile(hFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        pHeader,
                        0x1C,
                        NULL);

    if (!NT_SUCCESS(Status) ||
        ((DWORD)IoStatus.Information != 0x1C))
    {
        XapiSetLastNTError(NT_SUCCESS(Status) ? STATUS_INVALID_IMAGE_FORMAT : Status);
        return FALSE;
    }

    if (pHeader->cbHeader > XCONTENT_METADATA_HEADER_SIZE)
    {
        cbRead = XCONTENT_METADATA_HEADER_SIZE - 0x1C;
    }
    else
    {
        cbRead = pHeader->cbHeader - 0x1C;
    }

    ByteOffset.QuadPart = 0x1C;

    Status = NtReadFile(hFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        (PBYTE)pHeader + 0x1C,
                        cbRead,
                        &ByteOffset);

    if (!NT_SUCCESS(Status) ||
        ((DWORD)IoStatus.Information != cbRead))
    {
        XapiSetLastNTError(NT_SUCCESS(Status) ? STATUS_INVALID_IMAGE_FORMAT : Status);
        return FALSE;
    }

    if (pHeader->dwMagic != XCONTENT_METADATA_MAGIC)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    if (pHeader->cbHeader < XCONTENT_METADATA_HEADER_SIZE)
    {
        RtlZeroMemory((PBYTE)pHeader + pHeader->cbHeader,
                      XCONTENT_METADATA_HEADER_SIZE - pHeader->cbHeader);
    }

    if (fVerifySignature)
    {
        if (!XapiComputeContentHeaderSignature(hFile,
                                               pHeader,
                                               dwTitleId,
                                               rgbComputed))
        {
            return FALSE;
        }

        if (0 != memcmp(rgbComputed, pHeader->rgbHeaderSignature, XCONTENT_SIGNATURE_SIZE))
        {
            SetLastError(ERROR_INVALID_PASSWORD);
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
WINAPI
XapiVerifyAndLoadOptionalSectionData(
    IN HANDLE hFile,
    IN PXCONTENT_METADATA_HEADER pHeader,
    IN OUT PXCONTENT_SIGNATURES pSignatures
    )
{
    XCONTENT_SIGNATURE_FILE_HEADER SectionHeader;
    BYTE ShaContext[XC_SERVICE_SHA_CONTEXT_SIZE];
    BYTE rgbDigest[XCONTENT_SIGNATURE_SIZE];
    DWORD cbRead;

    if (0 == (pHeader->dwFlags & 0x80000000))
    {
        return TRUE;
    }

    if ((0 == pHeader->dwOptionalSectionOffset) ||
        (pHeader->cbOptionalSection != XCONTENT_OPTIONAL_SECTION_SIZE))
    {
        goto Error;
    }

    if (INVALID_SET_FILE_POINTER ==
        SetFilePointer(hFile, pHeader->dwOptionalSectionOffset, NULL, FILE_BEGIN))
    {
        goto Error;
    }

    if (!ReadFile(hFile,
                  &SectionHeader,
                  sizeof(SectionHeader),
                  &cbRead,
                  NULL) ||
        (cbRead != sizeof(SectionHeader)))
    {
        goto Error;
    }

    if ((SectionHeader.wTag != XCONTENT_OPTIONAL_SECTION_TAG) ||
        (SectionHeader.wVersion != XCONTENT_OPTIONAL_SECTION_VERSION))
    {
        goto Error;
    }

    XcSHAInit(ShaContext);
    XcSHAUpdate(ShaContext,
                (PBYTE)pHeader + 0x24,
                pHeader->cbHeader - 0x24);
    XcSHAUpdate(ShaContext, &SectionHeader, 0x0C);
    XcSHAFinal(ShaContext, rgbDigest);

    pSignatures->pvOptionalLive = pSignatures->pbSection;
    return TRUE;

Error:
    pSignatures->pvOptionalLive = NULL;
    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
}

DWORD
WINAPI
XapipGetAlternateTitleID(
    VOID
    )
{
    OBJECT_ATTRIBUTES Obja;
    HANDLE LinkHandle;
    OBJECT_STRING LinkTarget;
    CHAR szTarget[MAX_PATH];
    ULONG cchTarget;
    ULONG i;
    DWORD dwTitleId;

    InitializeObjectAttributes(&Obja,
                               (POBJECT_STRING)&WDrive,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    if (!NT_SUCCESS(NtOpenSymbolicLinkObject(&LinkHandle, &Obja)))
    {
        return 0;
    }

    LinkTarget.Buffer = szTarget;
    LinkTarget.Length = 0;
    LinkTarget.MaximumLength = sizeof(szTarget);

    if (!NT_SUCCESS(NtQuerySymbolicLinkObject(LinkHandle, &LinkTarget, NULL)))
    {
        NtClose(LinkHandle);
        return 0;
    }

    NtClose(LinkHandle);

    cchTarget = LinkTarget.Length / sizeof(CHAR);

    if ((cchTarget < 9) || (szTarget[cchTarget - 1] != '\\'))
    {
        return 0;
    }

    dwTitleId = 0;

    for (i = cchTarget - 8; i < cchTarget; i++)
    {
        CHAR ch;

        ch = szTarget[i];

        if ((ch >= '0') && (ch <= '9'))
        {
            dwTitleId = (dwTitleId << 4) + (DWORD)(ch - '0');
        }
        else if ((ch >= 'A') && (ch <= 'F'))
        {
            dwTitleId = (dwTitleId << 4) + (DWORD)(ch - 'A' + 10);
        }
        else if ((ch >= 'a') && (ch <= 'f'))
        {
            dwTitleId = (dwTitleId << 4) + (DWORD)(ch - 'a' + 10);
        }
        else
        {
            return 0;
        }
    }

    return dwTitleId;
}

BOOL
WINAPI
XGetContentInstallLocationFromIDs(
    IN DWORD dwTitleID,
    IN XOFFERING_ID xOfferingID,
    OUT LPSTR lpInstallDirectory
    )
{
    CHAR chDrive;

    if ((0 != dwTitleID) &&
        (dwTitleID != XeImageHeader()->Certificate->TitleID))
    {
        if (XapipGetAlternateTitleID() != dwTitleID)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        chDrive = 'W';
    }
    else
    {
        chDrive = 'T';
    }

    sprintf(lpInstallDirectory,
            XapipContentDirectoryTemplate,
            (DWORD)(xOfferingID >> 32),
            (DWORD)xOfferingID);

    lpInstallDirectory[0] = chDrive;
    return TRUE;
}

BOOL
WINAPI
XGetContentInstallLocation(
    IN DWORD dwTitleID,
    IN LPCSTR lpSourceMetadataFileName,
    OUT LPSTR lpInstallDirectory
    )
{
    HANDLE hFile;
    XCONTENT_METADATA_HEADER Header;
    DWORD dwResolvedTitleId;

    hFile = CreateFile(lpSourceMetadataFileName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        return FALSE;
    }

    if (!XapiLoadContentMetadataHeader(hFile, FALSE, 0, &Header))
    {
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    dwResolvedTitleId = dwTitleID;
    if (0 == dwResolvedTitleId)
    {
        dwResolvedTitleId = Header.dwTitleID;
    }

    return XGetContentInstallLocationFromIDs(dwResolvedTitleId,
                                             ((ULONGLONG)Header.dwOfferingIdHigh << 32) |
                                             Header.dwOfferingIdLow,
                                             lpInstallDirectory);
}

BOOL
WINAPI
XInstallContentSignaturesWithFileName(
    IN DWORD dwTitleID,
    IN DWORD dwInstallFlags,
    IN LPCSTR lpSourceMetadataFileName,
    IN LPCSTR lpDestinationMetadataFileName
    )
{
    HANDLE hSource;
    HANDLE hDest;
    DWORD cbSourceRemaining;
    DWORD cbChunk;
    PBYTE pbCopyBuffer;
    BOOL fSuccess;
    XCONTENT_METADATA_HEADER Header;
    BYTE rgbSignature[XCONTENT_SIGNATURE_SIZE];
    IO_STATUS_BLOCK IoStatus;
    FILE_DISPOSITION_INFORMATION DispositionInfo;

    hSource = INVALID_HANDLE_VALUE;
    hDest = INVALID_HANDLE_VALUE;
    pbCopyBuffer = NULL;
    fSuccess = FALSE;

    if (NULL != lpSourceMetadataFileName)
    {
        hSource = CreateFile(lpSourceMetadataFileName,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

        if (INVALID_HANDLE_VALUE == hSource)
        {
            goto Exit;
        }

        cbSourceRemaining = GetFileSize(hSource, NULL);
        if (INVALID_FILE_SIZE == cbSourceRemaining)
        {
            goto Exit;
        }

        hDest = CreateFile(lpDestinationMetadataFileName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (INVALID_HANDLE_VALUE == hDest)
        {
            goto Exit;
        }

        pbCopyBuffer = (PBYTE)MmAllocateSystemMemory(XCONTENT_INSTALL_COPY_CHUNK, 4);
        if (NULL == pbCopyBuffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Exit;
        }

        while (cbSourceRemaining)
        {
            cbChunk = min(cbSourceRemaining, XCONTENT_INSTALL_COPY_CHUNK);

            if (!ReadFile(hSource, pbCopyBuffer, cbChunk, &cbChunk, NULL) ||
                !WriteFile(hDest, pbCopyBuffer, cbChunk, &cbChunk, NULL))
            {
                goto Exit;
            }

            cbSourceRemaining -= cbChunk;
        }

        CloseHandle(hSource);
        hSource = INVALID_HANDLE_VALUE;

        MmFreeSystemMemory(pbCopyBuffer, XCONTENT_INSTALL_COPY_CHUNK);
        pbCopyBuffer = NULL;
    }
    else
    {
        hDest = CreateFile(lpDestinationMetadataFileName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (INVALID_HANDLE_VALUE == hDest)
        {
            goto Exit;
        }
    }

    if (!XapiLoadContentMetadataHeader(hDest,
                                       FALSE,
                                       dwTitleID,
                                       &Header))
    {
        goto Exit;
    }

    if (0 == dwTitleID)
    {
        dwTitleID = XeImageHeader()->Certificate->TitleID;
    }

    if (dwInstallFlags & 2)
    {
        Header.dwFlags |= 1;
    }
    else
    {
        Header.dwFlags &= ~1;
    }

    if (!XapiComputeContentHeaderSignature(hDest, &Header, dwTitleID, rgbSignature))
    {
        goto Exit;
    }

    memcpy(Header.rgbHeaderSignature, rgbSignature, XCONTENT_SIGNATURE_SIZE);

    {
        NTSTATUS Status;

        Status = NtWriteFile(hDest,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             &Header,
                             min(Header.cbHeader, XCONTENT_METADATA_HEADER_SIZE),
                             NULL);

        if (!NT_SUCCESS(Status))
        {
            XapiSetLastNTError(Status);
            goto Exit;
        }
    }

    NtClose(hDest);
    hDest = INVALID_HANDLE_VALUE;
    fSuccess = TRUE;

Exit:
    if (INVALID_HANDLE_VALUE != hSource)
    {
        CloseHandle(hSource);
    }

    if (NULL != pbCopyBuffer)
    {
        MmFreeSystemMemory(pbCopyBuffer, XCONTENT_INSTALL_COPY_CHUNK);
    }

    if ((INVALID_HANDLE_VALUE != hDest) && !fSuccess)
    {
        if (NULL != lpSourceMetadataFileName)
        {
            DispositionInfo.DeleteFile = TRUE;
            NtSetInformationFile(hDest,
                                 &IoStatus,
                                 &DispositionInfo,
                                 sizeof(DispositionInfo),
                                 FileDispositionInformation);
        }

        CloseHandle(hDest);
    }

    return fSuccess;
}

BOOL
WINAPI
XInstallContentSignaturesEx(
    IN DWORD dwTitleID,
    IN DWORD dwInstallFlags,
    IN LPCSTR lpSourceDirectory,
    IN LPCSTR lpDestinationDirectory
    )
{
    CHAR szMetadataFileName[MAX_PATH];

    XapiComputeContentMetadataFileName(lpDestinationDirectory, szMetadataFileName);

    return XInstallContentSignaturesWithFileName(dwTitleID,
                                                 dwInstallFlags,
                                                 lpSourceDirectory,
                                                 szMetadataFileName);
}

BOOL
WINAPI
XInstallContentSignatures(
    IN DWORD dwTitleID,
    IN LPCSTR lpSourceMetadataFileName,
    IN LPCSTR lpDestinationDirectory
    )
{
    return XInstallContentSignaturesEx(dwTitleID,
                                       2,
                                       lpSourceMetadataFileName,
                                       lpDestinationDirectory);
}

BOOL
WINAPI
XCreateContentSimple(
    IN DWORD dwTitleID,
    IN XOFFERING_ID xOfferingID,
    IN DWORD dwContentFlags,
    IN LPCWSTR lpContentName,
    IN LPCSTR lpDestinationDirectory
    )
{
    CHAR szMetadataFileName[MAX_PATH];
    HANDLE hFile;
    PXCONTENT_METADATA_HEADER pHeader;
    DWORD cbUnicode;
    DWORD cbAlloc;
    PBYTE pbUnicode;
    PWSTR pszWrite;
    BYTE ShaContext[XC_SERVICE_SHA_CONTEXT_SIZE];
    DWORD cbWritten;
    BOOL fSuccess;

    hFile = INVALID_HANDLE_VALUE;
    pHeader = NULL;
    fSuccess = FALSE;

    cbUnicode = (DWORD)(wcslen(lpContentName) * sizeof(WCHAR));
    cbAlloc = XCONTENT_METADATA_HEADER_SIZE + cbUnicode + 0x26;

    pHeader = (PXCONTENT_METADATA_HEADER)XMemAlloc(cbAlloc, XMEMALLOC_XAPI_ATTRIBUTES);
    if (NULL == pHeader)
    {
        return FALSE;
    }

    XapiComputeContentMetadataFileName(lpDestinationDirectory, szMetadataFileName);

    hFile = CreateFile(szMetadataFileName,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        goto Exit;
    }

    RtlZeroMemory(pHeader, cbAlloc);

    pHeader->dwMagic = XCONTENT_METADATA_MAGIC;
    pHeader->cbHeader = XCONTENT_METADATA_HEADER_SIZE;
    pHeader->dwVersion = 1;
    pHeader->dwFlags = 1;
    pHeader->dwTitleID = (0 != dwTitleID) ? dwTitleID : XeImageHeader()->Certificate->TitleID;
    pHeader->dwOfferingIdHigh = (DWORD)(xOfferingID >> 32);
    pHeader->dwOfferingIdLow = (DWORD)xOfferingID;
    pHeader->dwContentFlags = dwContentFlags;
    pHeader->dwUnicodeOffset = XCONTENT_METADATA_HEADER_SIZE;
    pHeader->cbUnicode = cbUnicode + 0x26;

    pbUnicode = (PBYTE)pHeader + XCONTENT_METADATA_HEADER_SIZE;
    pszWrite = (PWSTR)pbUnicode;
    *pszWrite++ = g_chUnicodeSignature;
    wcscpy(pszWrite, g_cszDefaultLanguage);
    pszWrite += wcslen(g_cszDefaultLanguage);
    wcscpy(pszWrite, g_cszCRLF);
    pszWrite += wcslen(g_cszCRLF);
    wcscpy(pszWrite, g_cszNameTag);
    pszWrite += wcslen(g_cszNameTag);
    *pszWrite++ = g_chEqual;
    wcscpy(pszWrite, lpContentName);
    pszWrite += wcslen(lpContentName);
    wcsncpy(pszWrite, g_cszCRLF, 2);

    XcSHAInit(ShaContext);
    XcSHAUpdate(ShaContext, pbUnicode, pHeader->cbUnicode);
    XcSHAFinal(ShaContext, pHeader->rgbUnicodeDigest);

    if (!XapiComputeContentHeaderSignature(NULL, pHeader, 0, pHeader->rgbHeaderSignature))
    {
        goto Exit;
    }

    if (!WriteFile(hFile, pHeader, cbAlloc, &cbWritten, NULL) ||
        (cbWritten != cbAlloc))
    {
        goto Exit;
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
    fSuccess = TRUE;

Exit:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        SetEndOfFile(hFile);
        CloseHandle(hFile);
    }

    XMemFree(pHeader, XMEMALLOC_XAPI_ATTRIBUTES);
    return fSuccess;
}

BOOL
WINAPI
XRemoveContent(
    IN LPCSTR lpDirectoryName
    )
{
    CHAR szMetadataFileName[MAX_PATH];
    NTSTATUS Status;

    XapiComputeContentMetadataFileName(lpDirectoryName, szMetadataFileName);
    DeleteFileA(szMetadataFileName);

    Status = XapiNukeDirectory(lpDirectoryName);
    if (Status < 0)
    {
        XapiSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

HANDLE
WINAPI
XLoadContentSignaturesWithFileName(
    IN DWORD dwTitleID,
    IN LPCSTR lpMetadataFileName
    )
{
    HANDLE hFile;
    XCONTENT_METADATA_HEADER Header;
    PXCONTENT_SIGNATURES pSignatures;
    DWORD cbFile;
    DWORD cbSection;
    DWORD cbRead;
    BYTE ShaContext[XC_SERVICE_SHA_CONTEXT_SIZE];
    BYTE rgbDigest[XCONTENT_SIGNATURE_SIZE];

    hFile = CreateFile(lpMetadataFileName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        return NULL;
    }

    cbFile = GetFileSize(hFile, NULL);
    if (INVALID_FILE_SIZE == cbFile)
    {
        goto Error;
    }

    if (0 == dwTitleID)
    {
        dwTitleID = XeImageHeader()->Certificate->TitleID;
    }

    if (!XapiLoadContentMetadataHeader(hFile, TRUE, dwTitleID, &Header))
    {
        goto Error;
    }

    if (0 == (Header.dwFlags & 1))
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        goto Error;
    }

    cbSection = cbFile - Header.cbHeader;
    pSignatures = (PXCONTENT_SIGNATURES)XMemAlloc(cbSection + 0x98,
                                                  XMEMALLOC_XAPI_ATTRIBUTES);
    if (NULL == pSignatures)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Error;
    }

    RtlZeroMemory(pSignatures, cbSection + 0x98);
    pSignatures->pbSection = (PBYTE)pSignatures + 0x28;
    pSignatures->pbTreeEnd = pSignatures->pbSection + XCONTENT_OPTIONAL_SECTION_SIZE;

    if (!XapiVerifyAndLoadOptionalSectionData(hFile, &Header, pSignatures))
    {
        goto ErrorFree;
    }

    if (SetFilePointer(hFile, Header.cbHeader, NULL, FILE_BEGIN) != (LONG)Header.cbHeader)
    {
        goto ErrorFree;
    }

    if (!ReadFile(hFile,
                  pSignatures->pbTreeEnd,
                  cbSection,
                  &cbRead,
                  NULL) ||
        (cbRead != cbSection))
    {
        goto ErrorFree;
    }

    pSignatures->pbDataEnd = pSignatures->pbTreeEnd + cbRead;
    pSignatures->pbTreeEnd = pSignatures->pbSection;

    XcSHAInit(ShaContext);
    XcSHAUpdate(ShaContext,
                pSignatures->pbSection,
                (DWORD)(pSignatures->pbDataEnd - pSignatures->pbSection));
    XcSHAFinal(ShaContext, rgbDigest);

    if (0 != memcmp(rgbDigest, Header.rgbUnicodeDigest, XCONTENT_SIGNATURE_SIZE))
    {
        goto ErrorFree;
    }

    pSignatures->dwMagic = XCONTENT_SIGNATURE_HANDLE_MAGIC;
    CloseHandle(hFile);
    return (HANDLE)pSignatures;

ErrorFree:
    if (NULL != pSignatures)
    {
        XMemFree(pSignatures, XMEMALLOC_XAPI_ATTRIBUTES);
    }

Error:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }

    return NULL;
}

HANDLE
WINAPI
XLoadContentSignaturesEx(
    IN DWORD dwTitleID,
    IN LPCSTR lpDirectoryName
    )
{
    CHAR szMetadataFileName[MAX_PATH];

    XapiComputeContentMetadataFileName(lpDirectoryName, szMetadataFileName);
    return XLoadContentSignaturesWithFileName(dwTitleID, szMetadataFileName);
}

BOOL
WINAPI
XLocateSignatureByIndex(
    IN HANDLE hSignature,
    IN DWORD dwSignatureIndex,
    OUT PBYTE *ppbSignatureData,
    OUT PDWORD pdwSignatureSize
    )
{
    PXCONTENT_SIGNATURES pSignatures;
    PXCONTENT_SIGNATURE_FILE_HEADER pFileHeader;
    PXCONTENT_SIGNATURE_INDEX pIndex;
    DWORD cPrimary;

    pSignatures = (PXCONTENT_SIGNATURES)hSignature;
    pFileHeader = (PXCONTENT_SIGNATURE_FILE_HEADER)pSignatures->pbSection;

    if (dwSignatureIndex < pFileHeader->cEntries)
    {
        cPrimary = *(PDWORD)pSignatures->pbSection;
        pIndex = (PXCONTENT_SIGNATURE_INDEX)((PBYTE)pSignatures->pbSection +
                                             cPrimary * 5 * sizeof(DWORD) +
                                             dwSignatureIndex * sizeof(XCONTENT_SIGNATURE_INDEX) +
                                             0x10);

        *ppbSignatureData = (PBYTE)pIndex + *(PDWORD)pIndex + cPrimary * 5 * sizeof(DWORD);
        *pdwSignatureSize = pIndex->dwDataSize;
        return TRUE;
    }

    if ((dwSignatureIndex - pFileHeader->cEntries) >= *(PDWORD)pSignatures->pbSection)
    {
        SetLastError(1168); // ERROR_NOT_FOUND
        return FALSE;
    }

    pIndex = (PXCONTENT_SIGNATURE_INDEX)((PBYTE)pSignatures->pbSection +
                                         (dwSignatureIndex * 5 + 3) * sizeof(DWORD));

    *ppbSignatureData = (PBYTE)pIndex;
    *pdwSignatureSize = 0x14;
    return TRUE;
}

BOOL
WINAPI
XLocateSignatureByNameEx(
    IN HANDLE hSignature,
    IN LPCSTR lpFileName,
    IN DWORD dwFileOffset,
    OUT PDWORD pdwDataSize,
    OUT PBYTE *ppbSignatureData,
    OUT PDWORD pdwSignatureSize
    )
{
    PXCONTENT_SIGNATURES pSignatures;
    PXCONTENT_SIGNATURE_FILE_HEADER pFileHeader;
    ANSI_STRING TargetName;
    ANSI_STRING NodeName;
    PUSHORT pNode;
    PBYTE pbStringTable;
    ULONG cchName;
    USHORT Left;
    USHORT Right;
    USHORT Middle;

    pSignatures = (PXCONTENT_SIGNATURES)hSignature;
    pFileHeader = (PXCONTENT_SIGNATURE_FILE_HEADER)pSignatures->pbSection;
    pbStringTable = pSignatures->pbSection + pFileHeader->dwStringTableOffset;

    RtlInitAnsiString(&TargetName, lpFileName);
    TargetName.Length = (USHORT)(strlen(lpFileName));

    pNode = (PUSHORT)pbStringTable;

    while (*pNode)
    {
        NodeName.Buffer = (PSTR)(pbStringTable + *(PUSHORT)(pNode + 3));
        NodeName.Length = *(PUSHORT)(pNode + 2);
        NodeName.MaximumLength = NodeName.Length;

        if (0 == RtlCompareString(&NodeName, &TargetName, TRUE))
        {
            break;
        }

        pNode = (PUSHORT)(pbStringTable + pNode[(*pNode < TargetName.Length) ? 0 : 1]);
    }

    if (0 == *pNode)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (0 == *pdwDataSize)
    {
        *pdwDataSize = *(PDWORD)(pNode + 4) - dwFileOffset;
    }

    Left = 0;
    Right = pNode[2];

    while (Left < Right)
    {
        PXCONTENT_SIGNATURE_INDEX pIndex;

        Middle = (USHORT)((Left + Right) / 2);
        pIndex = (PXCONTENT_SIGNATURE_INDEX)((PBYTE)pNode +
                                             Middle * sizeof(XCONTENT_SIGNATURE_INDEX) * 2);

        if (pIndex->dwDataOffset < dwFileOffset)
        {
            Left = Middle + 1;
        }
        else if (pIndex->dwDataOffset > dwFileOffset)
        {
            Right = Middle;
        }
        else if (*pdwDataSize <= pIndex->dwDataSize)
        {
            break;
        }
        else
        {
            Right = Middle;
        }
    }

    if (Left >= Right)
    {
        SetLastError(1168);
        return FALSE;
    }

    return XLocateSignatureByIndex(hSignature,
                                   *(PUSHORT)((PBYTE)pNode + Left * 8 + 0x0C),
                                   ppbSignatureData,
                                   pdwSignatureSize);
}

BOOL
WINAPI
XLocateSignatureByName(
    IN HANDLE hSignature,
    IN LPCSTR lpFileName,
    IN DWORD dwFileOffset,
    IN DWORD dwDataSize,
    OUT PBYTE *ppbSignatureData,
    OUT PDWORD pdwSignatureSize
    )
{
    return XLocateSignatureByNameEx(hSignature,
                                    lpFileName,
                                    dwFileOffset,
                                    &dwDataSize,
                                    ppbSignatureData,
                                    pdwSignatureSize);
}

BOOL
WINAPI
XLocateNextSignature(
    IN OUT HANDLE hSignature,
    OUT PSTR *ppszFileName,
    OUT PDWORD pdwFileOffset,
    OUT PDWORD pdwDataSize,
    OUT PDWORD pdwUnknown,
    OUT PBYTE *ppbSignatureData,
    OUT PDWORD pdwSignatureSize
    )
{
    PXCONTENT_SIGNATURES pSignatures;
    PUSHORT pNode;
    PBYTE pbStringTable;
    USHORT Left;
    USHORT Right;
    DWORD dwEnd;

    pSignatures = (PXCONTENT_SIGNATURES)hSignature;
    pNode = (PUSHORT)pSignatures->pbTreeEnd;
    pbStringTable = pSignatures->pbSection +
        ((PXCONTENT_SIGNATURE_FILE_HEADER)pSignatures->pbSection)->dwStringTableOffset;

    if ((PBYTE)pNode > pSignatures->pbDataEnd)
    {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }

    dwEnd = (DWORD)(pSignatures->pbDataEnd - pbStringTable);

    if (pNode[0])
    {
        dwEnd = min(dwEnd, pNode[0] + (DWORD)(pbStringTable - pSignatures->pbSection));
        pSignatures->pbDataEnd = (PBYTE)dwEnd;
    }

    if (pNode[1])
    {
        dwEnd = min((DWORD)(pSignatures->pbDataEnd - pbStringTable), pNode[1]);
    }

    if (NULL != ppszFileName)
    {
        *ppszFileName = (PSTR)(pbStringTable + pNode[2] * 5 * 2 + 0x0C);
    }

    if (NULL != pdwUnknown)
    {
        *pdwUnknown = pNode[3];
    }

    if (NULL != pdwDataSize)
    {
        *pdwDataSize = ((PDWORD)(pNode + 4))[0];
    }

    if (NULL != pdwFileOffset)
    {
        *pdwFileOffset = ((PDWORD)(pNode + 4))[1];
    }

    if (!XLocateSignatureByIndex(hSignature,
                                   pNode[4],
                                   ppbSignatureData,
                                   pdwSignatureSize))
    {
        return FALSE;
    }

    pSignatures->pbTreeEnd = (PBYTE)(((ULONG_PTR)(pNode + 4) + pNode[2] * 5 * 2 + 0x0F) & ~3);
    return TRUE;
}

BOOL
WINAPI
XCalculateContentSignature(
    IN LPBYTE pbData,
    IN DWORD dwDataSize,
    OUT LPBYTE pbSignature,
    IN OUT PDWORD pdwSignatureSize
    )
{
    BYTE ShaContext[XC_SERVICE_SHA_CONTEXT_SIZE];

    if ((NULL != pdwSignatureSize) && (*pdwSignatureSize < XCONTENT_SIGNATURE_SIZE))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    XcSHAInit(ShaContext);
    XcSHAUpdate(ShaContext, pbData, dwDataSize);

    if (NULL != pbSignature)
    {
        XcSHAFinal(ShaContext, pbSignature);
    }

    if (NULL != pdwSignatureSize)
    {
        *pdwSignatureSize = XCONTENT_SIGNATURE_SIZE;
    }

    return TRUE;
}

BOOL
WINAPI
XLocateLiveSignature(
    IN HANDLE hSignature,
    OUT PBYTE pbLiveDigest,
    OUT PBYTE pbSignature
    )
{
    PXCONTENT_SIGNATURES pSignatures;
    PUSHORT pLive;

    pSignatures = (PXCONTENT_SIGNATURES)hSignature;
    pLive = (PUSHORT)pSignatures->pvOptionalLive;

    if ((NULL == pLive) ||
        (pLive[0] != XCONTENT_OPTIONAL_SECTION_TAG) ||
        (pLive[1] != XCONTENT_OPTIONAL_SECTION_VERSION))
    {
        return FALSE;
    }

    *(PDWORD)pbLiveDigest = *(PDWORD)(pLive + 2);
    RtlCopyMemory(pbLiveDigest + 4, pLive + 4, 0x19 * sizeof(DWORD));
    RtlCopyMemory(pbSignature, (PBYTE)pSignatures + 0x14, XCONTENT_SIGNATURE_SIZE);
    return TRUE;
}

VOID
WINAPI
XCloseContentSignatures(
    IN HANDLE hSignature
    )
{
    PXCONTENT_SIGNATURES pSignatures;

    pSignatures = (PXCONTENT_SIGNATURES)hSignature;
    pSignatures->dwMagic = 0;
    XMemFree(pSignatures, XMEMALLOC_XAPI_ATTRIBUTES);
}
