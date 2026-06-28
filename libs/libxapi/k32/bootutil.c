#include "bridge_k32.h"
/*++

Copyright (c) Microsoft Corporation

Description:
    Boot-time title update utilities: dash partition mount, update
    detection, and launch/reboot helpers.

Module Name:

    bootutil.c

--*/

#include "basedll.h"
#pragma hdrstop

#include <xboxp.h>
#include <xlaunch.h>
#include <sha.h>
#include <dm.h>

NTSTATUS
__attribute__((__stdcall__))
XWriteTitleInfoNoReboot(
    PCOSTR pszLaunchPath,
    PCOSTR pszDDrivePath,
    DWORD dwLaunchDataType,
    DWORD dwTitleId,
    PLAUNCH_DATA pLaunchData
    );

//
// Exported drive and partition strings
//

static OCHAR s_yDrivePath[] = OTEXT("\\??\\Y:");
const OBJECT_STRING YDrive = CONSTANT_OBJECT_STRING(s_yDrivePath);

const OCHAR DashPartition[] =
    OTEXT("\\Device\\Harddisk0\\Partition2\\");

const OCHAR TitleRebootPath[] =
    OTEXT("\\Device\\Harddisk0\\Partition1\\TDATA\\%08x\\$u\\%s");

COBJECT_STRING DDrive = CONSTANT_OBJECT_STRING(OTEXT("\\??\\D:"));

static const CHAR g_szDVDDevicePrefix[] = "\\Device\\Cdrom0";

extern POBJECT_STRING XeImageFileName;

NTSTATUS
__attribute__((__stdcall__))
XWriteTitleInfoNoReboot(
    PCOSTR pszLaunchPath,
    PCOSTR pszDDrivePath,
    DWORD dwLaunchDataType,
    DWORD dwTitleId,
    PLAUNCH_DATA pLaunchData
    );

BOOL
__attribute__((__stdcall__))
XapiLoadContentMetadataHeader(
    IN HANDLE hFile,
    IN BOOL fVerifySignature,
    IN DWORD dwTitleId,
    OUT PVOID pvHeader
    );

static const CHAR g_szContentMetaUpdateFile[] = "contentmeta.xbx";
static const CHAR g_szDashUpdateXbe[] = "d:\\dashupdate.xbe";
static const CHAR g_szXboxDashXbe[] = "y:\\xboxdash.xbe";
static const CHAR g_szOnlineDashXbe[] = "y:\\XODash\\xonlinedash.xbe";

#define XBE_UPDATE_HEADER_READ_SIZE 0x178

#define UPDATE_INFO_BLOCK_SIZE  0x44

typedef struct _XONLINEUPDATE_BASE_VERSIONS
{
    DWORD rgdwVersions[16];
} XONLINEUPDATE_BASE_VERSIONS, *PXONLINEUPDATE_BASE_VERSIONS;

typedef struct _XAPILAUNCH_SAVED_DDATA
{
    WORD fExtended;
    WORD cbPath;
    DWORD dwTitleId;
    CHAR szPaths[1];
} XAPILAUNCH_SAVED_DDATA, *PXAPILAUNCH_SAVED_DDATA;

static DWORD
BootutilNtStatusToHresult(
    IN NTSTATUS Status
    )
{
    DWORD dwError;

    if (NT_SUCCESS(Status))
    {
        return NO_ERROR;
    }

    dwError = RtlNtStatusToDosError(Status);

    if ((int) dwError > 0)
    {
        dwError = (dwError & 0xFFFF) | 0x80070000;
    }

    return dwError;
}

BOOL
BuildUpdateFilePath(
    IN DWORD dwTitleId,
    IN PCSTR pszFileName,
    OUT PSTR pszPath,
    IN OUT PDWORD pcchPath
    )
{
    int cch;

    cch = _snprintf(pszPath, *pcchPath, "t:\\%u\\%s", dwTitleId, pszFileName);

    if (cch < 0)
    {
        PSTR pszEnd;
        int cchRequired;

        pszEnd = pszPath;
        while (*pszEnd++)
        {
            ;
        }

        cchRequired = (int)(pszEnd - pszPath) + 7;
        *pcchPath = (DWORD)cchRequired;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    *pcchPath = (DWORD)(cch + 1);
    return TRUE;
}

DWORD
__attribute__((__stdcall__))
XapipUpdateMountDashPartition(
    VOID
    )
{
    NTSTATUS Status;
    OBJECT_STRING VolString;
    OBJECT_STRING DriveString;

    RtlInitAnsiString((PANSI_STRING)&VolString, DashPartition);
    RtlInitAnsiString((PANSI_STRING)&DriveString, DashPartition);
    DriveString.Length -= sizeof(OCHAR);

    Status = XapiValidateDiskPartition(&VolString);

    if (NT_SUCCESS(Status))
    {
        IoDeleteSymbolicLink((POBJECT_STRING)&YDrive);
        Status = IoCreateSymbolicLink((POBJECT_STRING)&YDrive, &DriveString);
    }

    return BootutilNtStatusToHresult(Status);
}

DWORD
__attribute__((__stdcall__))
XoUpdateLoadXBEInfo(
    IN PCSTR pszXbePath,
    OUT PXBEIMAGE_CERTIFICATE pCertificate
    )
{
    HANDLE hFile;
    DWORD dwError;
    XBEIMAGE_HEADER Header;
    DWORD cbRead;

    hFile = CreateFile(pszXbePath,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwError = GetLastError();

        if ((int) dwError > 0)
        {
            dwError = (dwError & 0xFFFF) | 0x80070000;
        }

        return dwError;
    }

    dwError = NO_ERROR;

    if (!ReadFile(hFile, &Header, XBE_UPDATE_HEADER_READ_SIZE, &cbRead, NULL) ||
        (cbRead != XBE_UPDATE_HEADER_READ_SIZE))
    {
        goto Error;
    }

    if (Header.SizeOfImage < Header.SizeOfHeaders)
    {
        goto InvalidData;
    }

    if (SetFilePointer(hFile, Header.SizeOfHeaders, NULL, FILE_BEGIN) != (LONG)Header.SizeOfHeaders)
    {
        goto Error;
    }

    if (!ReadFile(hFile,
                  pCertificate,
                  sizeof(XBEIMAGE_CERTIFICATE),
                  &cbRead,
                  NULL) ||
        (cbRead != sizeof(XBEIMAGE_CERTIFICATE)))
    {
        goto Error;
    }

    goto Exit;

InvalidData:
    SetLastError(ERROR_INVALID_DATA);

Error:
    dwError = GetLastError();

    if ((int) dwError > 0)
    {
        dwError = (dwError & 0xFFFF) | 0x80070000;
    }

Exit:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }

    return dwError;
}

VOID
__attribute__((__stdcall__))
XoRebootToUpdaterWhilePreservingDDrive(
    IN PCSTR pszUpdaterPath,
    IN PLAUNCH_DATA pLaunchData,
    IN DWORD dwLaunchDataType
    )
{
    OBJECT_ATTRIBUTES Obja;
    HANDLE LinkHandle;
    OBJECT_STRING LinkTarget;
    CHAR szLinkTarget[MAX_PATH * 2];
    CHAR szDDrivePath[MAX_LAUNCH_PATH];
    ULONG cchCopy;

    szDDrivePath[0] = '\0';

    InitializeObjectAttributes(&Obja,
                               (POBJECT_STRING)&DDrive,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    if (NT_SUCCESS(NtOpenSymbolicLinkObject(&LinkHandle, &Obja)))
    {
        LinkTarget.Buffer = szLinkTarget;
        LinkTarget.Length = 0;
        LinkTarget.MaximumLength = sizeof(szLinkTarget);

        if (NT_SUCCESS(NtQuerySymbolicLinkObject(LinkHandle, &LinkTarget, NULL)))
        {
            cchCopy = (LinkTarget.Length / sizeof(CHAR)) + 1;

            if (cchCopy > MAX_LAUNCH_PATH)
            {
                cchCopy = MAX_LAUNCH_PATH;
            }

            lstrcpynA(szDDrivePath, szLinkTarget, cchCopy);
        }

        NtClose(LinkHandle);
    }

    if ('\0' == szDDrivePath[0])
    {
        lstrcpynA(szDDrivePath,
                  g_szDVDDevicePrefix,
                  ARRAYSIZE(szDDrivePath));
    }

    XWriteTitleInfoAndRebootA(pszUpdaterPath + 3,
                              szDDrivePath,
                              dwLaunchDataType,
                              XeImageHeader()->Certificate->TitleID,
                              pLaunchData);
}

BOOL
__attribute__((__stdcall__))
XoUpdateGetSavedDataFromLaunchPage(
    OUT PSTR *ppszDDrivePath,
    OUT PDWORD pdwTitleId,
    OUT PLAUNCH_DATA pLaunchData
    )
{
    PLAUNCH_DATA_PAGE pPage;
    PXAPILAUNCH_SAVED_DDATA pSaved;

    *ppszDDrivePath = NULL;
    *pdwTitleId = (DWORD)-1;

    if (NULL == LaunchDataPage)
    {
        return FALSE;
    }

    pPage = LaunchDataPage;
    pSaved = (PXAPILAUNCH_SAVED_DDATA)((PBYTE)pPage + FIELD_OFFSET(LAUNCH_DATA_PAGE, Pad));

    if (pSaved->fExtended & 1)
    {
        *pdwTitleId = *(PDWORD)((PBYTE)pSaved + 4);
        *ppszDDrivePath = (PSTR)((PBYTE)pSaved + 8);
    }
    else
    {
        *pdwTitleId = pPage->Header.dwLaunchDataType;
        *ppszDDrivePath = pPage->Header.szLaunchPath;
    }

    if ((DWORD)-1 != *pdwTitleId)
    {
        memcpy(pLaunchData, pPage->LaunchData, sizeof(LAUNCH_DATA));
    }

    return (pSaved->fExtended & 1) ? TRUE : FALSE;
}

VOID
__attribute__((__stdcall__))
XapipUpdateGetCurrentDDriveMapping(
    OUT PSTR pszDDrivePath,
    IN DWORD cchDDrivePathMax
    )
{
    OBJECT_ATTRIBUTES Obja;
    HANDLE LinkHandle;
    OBJECT_STRING LinkTarget;
    CHAR szLinkTarget[MAX_PATH * 2];
    ULONG cchCopy;

    pszDDrivePath[0] = '\0';

    InitializeObjectAttributes(&Obja,
                               (POBJECT_STRING)&DDrive,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    if (NT_SUCCESS(NtOpenSymbolicLinkObject(&LinkHandle, &Obja)))
    {
        LinkTarget.Buffer = szLinkTarget;
        LinkTarget.Length = 0;
        LinkTarget.MaximumLength = sizeof(szLinkTarget);

        if (NT_SUCCESS(NtQuerySymbolicLinkObject(LinkHandle, &LinkTarget, NULL)))
        {
            cchCopy = (LinkTarget.Length / sizeof(CHAR)) + 1;

            if (cchCopy > cchDDrivePathMax)
            {
                cchCopy = cchDDrivePathMax;
            }

            lstrcpynA(pszDDrivePath, szLinkTarget, cchCopy);
        }

        NtClose(LinkHandle);
    }

    if ('\0' == pszDDrivePath[0])
    {
        lstrcpynA(pszDDrivePath,
                  g_szDVDDevicePrefix,
                  cchDDrivePathMax);
    }
}

VOID
__attribute__((__stdcall__))
XapipUpdateSaveDDriveMappingToLaunchPage(
    IN PCSTR pszLaunchPath,
    IN PCSTR pszDDrivePath
    )
{
    PLAUNCH_DATA_PAGE pPage;
    PXAPILAUNCH_SAVED_DDATA pSaved;
    int cchLaunch;
    int cchDDrive;
    int cbTotal;
    PSTR pszDest;

    pPage = LaunchDataPage;
    pSaved = (PXAPILAUNCH_SAVED_DDATA)((PBYTE)pPage + FIELD_OFFSET(LAUNCH_DATA_PAGE, Pad));

    cchLaunch = strlen(pszLaunchPath);
    cchDDrive = strlen(pszDDrivePath);
    cbTotal = cchLaunch + cchDDrive + 8;

    if (cbTotal >= (int)sizeof(pPage->Pad))
    {
        return;
    }

    pSaved->fExtended = 1;
    pSaved->cbPath = (WORD)(cchLaunch + 1);
    pSaved->dwTitleId = pPage->Header.dwLaunchDataType;

    if ((DWORD)-1 == pSaved->dwTitleId)
    {
        pSaved->dwTitleId = 0;
    }

    pszDest = (PSTR)((PBYTE)pSaved + 8);
    strcpy(pszDest, pszLaunchPath);
    strcpy(pszDest + cchLaunch + 1, pszDDrivePath);
}

typedef struct _XCONTENT_METADATA_HEADER_MIN
{
    BYTE rgbHeaderHmac[20];
    DWORD dwMagic;
    DWORD cbHeader;
    DWORD dwVersion;
    DWORD dwFlags;
} XCONTENT_METADATA_HEADER_MIN, *PXCONTENT_METADATA_HEADER_MIN;

DWORD
__attribute__((__stdcall__))
XapipUpdateDetectAndVerify(
    IN DWORD dwTitleId,
    IN PCSTR pszFileName,
    IN PDWORD pdwAllowedVersions,
    IN DWORD cVersions
    )
{
    CHAR szPath[MAX_PATH];
    DWORD cchPath;
    HANDLE hFile;
    DWORD dwResult;
    BYTE rgbHeader[0x88];
    BYTE rgbUpdateInfo[UPDATE_INFO_BLOCK_SIZE];
    DWORD cbRead;
    BYTE ShaContext[XC_SERVICE_SHA_CONTEXT_SIZE];
    BYTE rgbDigest[A_SHA_DIGEST_LEN];
    XBEIMAGE_CERTIFICATE Certificate;
    DWORD i;

    cchPath = sizeof(szPath);
    if (!BuildUpdateFilePath(dwTitleId, g_szContentMetaUpdateFile, szPath, &cchPath))
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    hFile = CreateFile(szPath,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    dwResult = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        return dwResult;
    }

    if (!XapiLoadContentMetadataHeader(hFile, TRUE, dwTitleId, rgbHeader))
    {
        goto Exit;
    }

    if (0 == (((PXCONTENT_METADATA_HEADER_MIN)rgbHeader)->dwFlags & 1))
    {
        goto Exit;
    }

    ZeroMemory(rgbUpdateInfo, sizeof(rgbUpdateInfo));

    if (!ReadFile(hFile, rgbUpdateInfo, UPDATE_INFO_BLOCK_SIZE, &cbRead, NULL) ||
        (cbRead != UPDATE_INFO_BLOCK_SIZE))
    {
        goto Exit;
    }

    if (*(PDWORD)(rgbUpdateInfo + 0x10) != UPDATE_INFO_BLOCK_SIZE)
    {
        goto Exit;
    }

    XcSHAInit(ShaContext);
    XcSHAUpdate(ShaContext, rgbUpdateInfo, UPDATE_INFO_BLOCK_SIZE);
    XcSHAFinal(ShaContext, rgbDigest);

    if (0 != memcmp(rgbDigest, rgbUpdateInfo + 0x24, A_SHA_DIGEST_LEN))
    {
        goto Exit;
    }

    for (i = 0; i < *(PDWORD)(rgbUpdateInfo + 0x0C); i++)
    {
        if (pdwAllowedVersions[i] == *(PDWORD)(rgbUpdateInfo + 0x10 + i * sizeof(DWORD)))
        {
            break;
        }
    }

    if (i >= *(PDWORD)(rgbUpdateInfo + 0x0C))
    {
        goto Exit;
    }

    cchPath = sizeof(szPath);
    if (!BuildUpdateFilePath(dwTitleId, pszFileName, szPath, &cchPath))
    {
        goto Exit;
    }

    if ((int)XoUpdateLoadXBEInfo(szPath, &Certificate) < 0)
    {
        goto Exit;
    }

    if (Certificate.TitleID != dwTitleId)
    {
        goto Exit;
    }

    dwResult = NO_ERROR;

Exit:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }

    if (NO_ERROR != dwResult)
    {
        dwResult = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    return dwResult;
}

VOID
__attribute__((__stdcall__))
XapipUpdateDashIfNecessary(
    IN PLAUNCH_DATA pLaunchData
    )
{
    XBEIMAGE_CERTIFICATE DashUpdateCert;
    XBEIMAGE_CERTIFICATE DashCert;
    XBEIMAGE_CERTIFICATE OnlineDashCert;

    XapipUpdateMountDashPartition();

    if ((int)XoUpdateLoadXBEInfo(g_szDashUpdateXbe, &DashUpdateCert) < 0)
    {
        return;
    }

    if ((int)XoUpdateLoadXBEInfo(g_szXboxDashXbe, &DashCert) < 0)
    {
        goto RebootToUpdater;
    }

    if (DashCert.TimeDateStamp < DashUpdateCert.TimeDateStamp)
    {
        goto RebootToUpdater;
    }

    if ((int)XoUpdateLoadXBEInfo(g_szOnlineDashXbe, &OnlineDashCert) < 0)
    {
        goto RebootToUpdater;
    }

    if (OnlineDashCert.TimeDateStamp < DashUpdateCert.TimeDateStamp)
    {
        goto RebootToUpdater;
    }

    return;

RebootToUpdater:
    XoRebootToUpdaterWhilePreservingDDrive(g_szDashUpdateXbe,
                                          pLaunchData,
                                          LDT_TITLE_UPDATE);
}

DWORD
__attribute__((__stdcall__))
XapipLaunchNewImageInternal(
    IN PCSTR pszLaunchPath,
    IN PLAUNCH_DATA pLaunchData,
    IN DWORD dwFlags,
    IN DWORD dwTitleId,
    IN PVOID pvUnused
    )
{
    CHAR szDDrivePath[MAX_LAUNCH_PATH];
    CHAR szLaunchPath[MAX_LAUNCH_PATH];
    PCSTR pszPath;
    DWORD dwLaunchDataType;
    DWORD dwLaunchTitleId;
    BOOL fPreserveDDrive;
    BOOL fTitlePathType;
    NTSTATUS Status;
    UNREFERENCED_PARAMETER(pvUnused);

    if (NULL == pszLaunchPath)
    {
        dwLaunchDataType = (NULL != pLaunchData) ? LDT_LAUNCH_DASHBOARD : LDT_NONE;

        return XWriteTitleInfoAndRebootA(NULL,
                                         NULL,
                                         dwLaunchDataType,
                                         XeImageHeader()->Certificate->TitleID,
                                         pLaunchData);
    }

    pszPath = pszLaunchPath;
    fPreserveDDrive = FALSE;
    fTitlePathType = FALSE;

    XapipUpdateGetCurrentDDriveMapping(szDDrivePath, ARRAYSIZE(szDDrivePath));

    if ((('D' == pszPath[0]) || ('d' == pszPath[0])) &&
        (':' == pszPath[1]) &&
        ('\\' == pszPath[2]))
    {
        pszPath += 3;
        fPreserveDDrive = TRUE;
        fTitlePathType = TRUE;
    }
    else if ((('T' == pszPath[0]) || ('t' == pszPath[0])) &&
             (':' == pszPath[1]) &&
             ('\\' == pszPath[2]) &&
             ('$' == pszPath[3]) &&
             (('U' == pszPath[4]) || ('u' == pszPath[4])) &&
             ('\\' == pszPath[5]))
    {
        pszPath += 6;

        _snprintf(szLaunchPath,
                  ARRAYSIZE(szLaunchPath),
                  TitleRebootPath,
                  XeImageHeader()->Certificate->TitleID,
                  pszPath);
        pszPath = szLaunchPath;
        fPreserveDDrive = TRUE;
        fTitlePathType = TRUE;
    }
    else if ((dwFlags & 2) &&
             (('Y' == pszPath[0]) || ('y' == pszPath[0])) &&
             (':' == pszPath[1]) &&
             ('\\' == pszPath[2]))
    {
        pszPath += 3;

        _snprintf(szLaunchPath,
                  ARRAYSIZE(szLaunchPath),
                  "%s%s",
                  DashPartition,
                  pszPath);
        pszPath = szLaunchPath;
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwLaunchDataType = fTitlePathType ? 1 : 0;
    dwLaunchTitleId = (NULL != pLaunchData) ? dwTitleId : (DWORD)-1;

    Status = XWriteTitleInfoNoReboot(pszPath,
                                     NULL,
                                     dwLaunchDataType,
                                     dwLaunchTitleId,
                                     pLaunchData);

    if (fPreserveDDrive && (NULL == pLaunchData))
    {
        XapipUpdateSaveDDriveMappingToLaunchPage(pszPath, szDDrivePath);
    }

    if (NT_SUCCESS(Status))
    {
        DmTell(DMTELL_REBOOT, NULL);
        HalReturnToFirmware(HalQuickRebootRoutine);
    }

    return RtlNtStatusToDosError(Status);
}

VOID
__attribute__((__stdcall__))
XapipUpdateRebootIfNecessary(
    VOID
    )
{
    DWORD dwTitleId;
    LAUNCH_DATA LaunchData;
    PSTR pszDDrivePath;
    CHAR szDDrivePath[MAX_LAUNCH_PATH];
    CHAR szRelativePath[MAX_PATH];
    CHAR szUpdatePath[MAX_PATH];
    POBJECT_STRING pImageName;
    ULONG cchPrefix;
    ULONG cchImage;
    BOOL fHadSavedData;
    PXONLINEUPDATE_BASE_VERSIONS pVersions;

    dwTitleId = XeImageHeader()->Certificate->TitleID;

    if (0xFFFE0000 == dwTitleId)
    {
        return;
    }

    fHadSavedData = XoUpdateGetSavedDataFromLaunchPage(&pszDDrivePath,
                                                       &dwTitleId,
                                                       &LaunchData);

    if (!fHadSavedData)
    {
        XapipUpdateGetCurrentDDriveMapping(szDDrivePath, ARRAYSIZE(szDDrivePath));
        pszDDrivePath = szDDrivePath;
    }

    if ((DWORD)-1 == dwTitleId)
    {
        return;
    }

    cchPrefix = strlen(pszDDrivePath);
    pImageName = XeImageFileName;
    cchImage = pImageName->Length / sizeof(CHAR);

    if (cchImage <= cchPrefix)
    {
        return;
    }

    if (0 != _strnicmp(pszDDrivePath, pImageName->Buffer, cchPrefix))
    {
        return;
    }

    strncpy(szRelativePath,
            pImageName->Buffer + cchPrefix,
            min(ARRAYSIZE(szRelativePath) - 1, cchImage - cchPrefix));
    szRelativePath[cchImage - cchPrefix] = '\0';

    if (_snprintf(szUpdatePath,
                  ARRAYSIZE(szUpdatePath),
                  "t:\\%u\\%s",
                  dwTitleId,
                  szRelativePath) < 0)
    {
        return;
    }

    pVersions = (PXONLINEUPDATE_BASE_VERSIONS)((PBYTE)XeImageHeader() + 0xAC);

    if ((int)XapipUpdateDetectAndVerify(dwTitleId,
                                        szRelativePath,
                                        pVersions->rgdwVersions,
                                        ARRAYSIZE(pVersions->rgdwVersions)) < 0)
    {
        goto RestoreDDrive;
    }

    XapipLaunchNewImageInternal(szUpdatePath,
                                fHadSavedData ? &LaunchData : NULL,
                                0,
                                dwTitleId,
                                NULL);

RestoreDDrive:
    if (fHadSavedData)
    {
        OBJECT_STRING LinkTarget;
        USHORT cchLink;

        cchLink = (USHORT)strlen(pszDDrivePath);

        IoDeleteSymbolicLink((POBJECT_STRING)&DDrive);

        LinkTarget.Buffer = pszDDrivePath;
        LinkTarget.Length = cchLink * sizeof(OCHAR);
        LinkTarget.MaximumLength = LinkTarget.Length;

        IoCreateSymbolicLink((POBJECT_STRING)&DDrive, &LinkTarget);
    }
}
