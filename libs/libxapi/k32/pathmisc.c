#include "bridge_k32.h"
/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Volume, path, and utility-drive services: GetDiskFreeSpaceEx and
 * GetVolumeInformation, the cache/utility-drive management APIs (mount, format,
 * swap, and select the primary Z: and secondary N: partitions), alternate-title
 * mounting, and disk cluster/sector-size queries.
 */

#include "basedll.h"
#include <xboxp.h>
#include <xdisk.h>
#include <xconfig.h>
#include "xmeta.h"
#include "fat.h"

static const OBJECT_STRING ZDrive      = CONSTANT_OBJECT_STRING( OTEXT("\\??\\Z:") );
static const OBJECT_STRING NDrive      = CONSTANT_OBJECT_STRING( OTEXT("\\??\\N:") );
static const OCHAR CacheDriveFormat[]  = OTEXT("\\Device\\Harddisk0\\Partition%d\\");

//
// Cache-DB slot indices for the primary (Z:) and secondary (N:) utility drives.
// XapiSelectCachePartition records the slot it wrote the Z: entry into; the
// secondary-drive APIs need it to locate Z:'s partition in the on-disk database.
// The -1 sentinel means "not mounted"; a BSS zero would falsely read as slot 0,
// so these are statically initialised.
//
static ULONG g_iZDriveDBIndex = (ULONG)-1;
static ULONG g_iNDriveDBIndex = (ULONG)-1;
static COBJECT_STRING WDrive           = CONSTANT_OBJECT_STRING( OTEXT("\\??\\W:") );
static COBJECT_STRING XDrive           = CONSTANT_OBJECT_STRING( OTEXT("\\??\\X:") );

#ifdef XAPILIBP

extern XAPI_MU_INFO XapiMuInfo;

#else  // XAPILIBP

XAPI_MU_INFO XapiMuInfo = {0};
#if DBG
BOOL g_fMountedUtilityDrive = FALSE;
#endif // DBG

#endif // XAPILIBP

//
//  Define the FAT32 X-Box cache db sector
//

typedef struct _XBOX_CACHE_DB_SECTOR {
    ULONG SectorBeginSignature;                     // offset = 0x000   0
    ULONG Version;                                  // offset = 0x004   4
    UCHAR Data[496];                                // offset = 0x008   8
    ULONG SectorEndSignature;                       // offset = 0x1fc 508
} XBOX_CACHE_DB_SECTOR, *PXBOX_CACHE_DB_SECTOR;

#define XBOX_HD_SECTOR_SIZE                    512

#define XBOX_CACHE_DB_DATA_SIZE                (sizeof(((PXBOX_CACHE_DB_SECTOR) 0)->Data))

#define XBOX_CACHE_DB_SECTOR_BEGIN_SIGNATURE   0x97315286
#define XBOX_CACHE_DB_SECTOR_END_SIGNATURE     0xAA550000
#define XBOX_CACHE_DB_CUR_VERSION              0x00000002
#define XBOX_CACHE_DB_MAX_ENTRY_COUNT          (XBOX_CACHE_DB_DATA_SIZE / sizeof(X_CACHE_DB_ENTRY))

#ifndef XAPILIBP

WINBASEAPI
BOOL
__attribute__((__stdcall__))
GetDiskFreeSpaceEx(
    PCOSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    OBJECT_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION NormalSizeInfo;

    ULARGE_INTEGER BytesPerAllocationUnit;
    ULARGE_INTEGER FreeBytesAvailableToCaller;
    ULARGE_INTEGER TotalNumberOfBytes;

    RIP_ON_NOT_TRUE("GetDiskFreeSpaceEx()", ARGUMENT_PRESENT(lpDirectoryName));

    RtlInitObjectString(&FileName, lpDirectoryName);

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        ObDosDevicesDirectory(),
        NULL
        );

    //
    // Open the file
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_FREE_SPACE_QUERY
                );
    if ( !NT_SUCCESS(Status) ) {
        XapiSetLastNTError(Status);
        if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            }
        return FALSE;
        }

    //
    // Determine the size parameters of the volume.
    //

    Status = NtQueryVolumeInformationFile(
                Handle,
                &IoStatusBlock,
                &NormalSizeInfo,
                sizeof(NormalSizeInfo),
                FileFsSizeInformation
                );
    NtClose(Handle);
    if ( !NT_SUCCESS(Status) ) {
        XapiSetLastNTError(Status);
        return FALSE;
        }

    BytesPerAllocationUnit.QuadPart =
        NormalSizeInfo.BytesPerSector * NormalSizeInfo.SectorsPerAllocationUnit;

    FreeBytesAvailableToCaller.QuadPart =
        BytesPerAllocationUnit.QuadPart * NormalSizeInfo.AvailableAllocationUnits.QuadPart;

    TotalNumberOfBytes.QuadPart =
        BytesPerAllocationUnit.QuadPart * NormalSizeInfo.TotalAllocationUnits.QuadPart;

    if ( ARGUMENT_PRESENT(lpFreeBytesAvailableToCaller) ) {
        lpFreeBytesAvailableToCaller->QuadPart = FreeBytesAvailableToCaller.QuadPart;
        }
    if ( ARGUMENT_PRESENT(lpTotalNumberOfBytes) ) {
        lpTotalNumberOfBytes->QuadPart = TotalNumberOfBytes.QuadPart;
        }
    if ( ARGUMENT_PRESENT(lpTotalNumberOfFreeBytes) ) {
        lpTotalNumberOfFreeBytes->QuadPart = FreeBytesAvailableToCaller.QuadPart;
        }

    return TRUE;
}


BOOL
APIENTRY
GetVolumeInformation(
    PCOSTR lpRootPathName,
    POSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    POSTR lpFileSystemNameBuffer,
    DWORD nFileSystemNameSize
    )

/*++

Routine Description:

    This function returns information about the file system whose root
    directory is specified.

Arguments:

    lpRootPathName - An optional parameter, that if specified, supplies
        the root directory of the file system that information is to be
        returned about.  If this parameter is not specified, then the
        root of the current directory is used.

    lpVolumeNameBuffer - An optional parameter that if specified returns
        the name of the specified volume.

    nVolumeNameSize - Supplies the length of the volume name buffer.
        This parameter is ignored if the volume name buffer is not
        supplied.

    lpVolumeSerialNumber - An optional parameter that if specified
        points to a DWORD.  The DWORD contains the 32-bit of the volume
        serial number.

    lpMaximumComponentLength - An optional parameter that if specified
        returns the maximum length of a filename component supported by
        the specified file system.  A filename component is that portion
        of a filename between pathname seperators.

    lpFileSystemFlags - An optional parameter that if specified returns
        flags associated with the specified file system.

        lpFileSystemFlags Flags:

            FS_CASE_IS_PRESERVED - Indicates that the case of file names
                is preserved when the name is placed on disk.

            FS_CASE_SENSITIVE - Indicates that the file system supports
                case sensitive file name lookup.

            FS_UNICODE_STORED_ON_DISK - Indicates that the file system
                supports unicode in file names as they appear on disk.

    lpFileSystemNameBuffer - An optional parameter that if specified returns
        the name for the specified file system (e.g. FAT, HPFS...).

    nFileSystemNameSize - Supplies the length of the file system name
        buffer.  This parameter is ignored if the file system name
        buffer is not supplied.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    OBJECT_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FS_ATTRIBUTE_INFORMATION AttributeInfo;
    PFILE_FS_VOLUME_INFORMATION VolumeInfo;
    ULONG AttributeInfoLength;
    ULONG VolumeInfoLength;
    BOOL rv;

    rv = FALSE;

    nVolumeNameSize *= 2;
    nFileSystemNameSize *= 2;

    RIP_ON_NOT_TRUE("GetVolumeInformation()", ARGUMENT_PRESENT(lpRootPathName));

    RtlInitObjectString(&FileName, lpRootPathName);

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        ObDosDevicesDirectory(),
        NULL
        );

    AttributeInfo = NULL;
    VolumeInfo = NULL;

    //
    // Open the file
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                );
    if ( !NT_SUCCESS(Status) ) {
        XapiSetLastNTError(Status);
        return FALSE;
        }

    if ( ARGUMENT_PRESENT(lpVolumeNameBuffer) ||
         ARGUMENT_PRESENT(lpVolumeSerialNumber) ) {
        if ( ARGUMENT_PRESENT(lpVolumeNameBuffer) ) {
            VolumeInfoLength = sizeof(*VolumeInfo)+nVolumeNameSize;
            }
        else {
            VolumeInfoLength = sizeof(*VolumeInfo)+MAX_PATH;
            }
        VolumeInfo = RtlAllocateHeap(XapiProcessHeap, 0, VolumeInfoLength);

        if ( !VolumeInfo ) {
            NtClose(Handle);
            XapiSetLastNTError(STATUS_NO_MEMORY);
            return FALSE;
            }
        }

    if ( ARGUMENT_PRESENT(lpFileSystemNameBuffer) ||
         ARGUMENT_PRESENT(lpMaximumComponentLength) ||
         ARGUMENT_PRESENT(lpFileSystemFlags) ) {
        if ( ARGUMENT_PRESENT(lpFileSystemNameBuffer) ) {
            AttributeInfoLength = sizeof(*AttributeInfo) + nFileSystemNameSize;
            }
        else {
            AttributeInfoLength = sizeof(*AttributeInfo) + MAX_PATH;
            }
        AttributeInfo = RtlAllocateHeap(XapiProcessHeap, 0, AttributeInfoLength);
        if ( !AttributeInfo ) {
            NtClose(Handle);
            if ( VolumeInfo ) {
                RtlFreeHeap(XapiProcessHeap, 0,VolumeInfo);
                }
            XapiSetLastNTError(STATUS_NO_MEMORY);
            return FALSE;
            }
        }

    try {
        if ( VolumeInfo ) {
            Status = NtQueryVolumeInformationFile(
                        Handle,
                        &IoStatusBlock,
                        VolumeInfo,
                        VolumeInfoLength,
                        FileFsVolumeInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                XapiSetLastNTError(Status);
                rv = FALSE;
                goto finally_exit;
                }
            }

        if ( AttributeInfo ) {
            Status = NtQueryVolumeInformationFile(
                        Handle,
                        &IoStatusBlock,
                        AttributeInfo,
                        AttributeInfoLength,
                        FileFsAttributeInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                XapiSetLastNTError(Status);
                rv = FALSE;
                goto finally_exit;
                }
            }
        try {

            if ( ARGUMENT_PRESENT(lpVolumeNameBuffer) ) {
                if ( VolumeInfo->VolumeLabelLength >= nVolumeNameSize ) {
                    SetLastError(ERROR_BAD_LENGTH);
                    rv = FALSE;
                    goto finally_exit;
                    }
                else {
                    RtlMoveMemory( lpVolumeNameBuffer,
                                   VolumeInfo->VolumeLabel,
                                   VolumeInfo->VolumeLabelLength );

                    *(lpVolumeNameBuffer + (VolumeInfo->VolumeLabelLength / sizeof(OCHAR))) = OBJECT_NULL;
                    }
                }

            if ( ARGUMENT_PRESENT(lpVolumeSerialNumber) ) {
                *lpVolumeSerialNumber = VolumeInfo->VolumeSerialNumber;
                }

            if ( ARGUMENT_PRESENT(lpFileSystemNameBuffer) ) {

                if ( AttributeInfo->FileSystemNameLength >= nFileSystemNameSize ) {
                    SetLastError(ERROR_BAD_LENGTH);
                    rv = FALSE;
                    goto finally_exit;
                    }
                else {
                    RtlMoveMemory( lpFileSystemNameBuffer,
                                   AttributeInfo->FileSystemName,
                                   AttributeInfo->FileSystemNameLength );

                    *(lpFileSystemNameBuffer + (AttributeInfo->FileSystemNameLength / sizeof(OCHAR))) = OBJECT_NULL;
                    }
                }

            if ( ARGUMENT_PRESENT(lpMaximumComponentLength) ) {
                *lpMaximumComponentLength = AttributeInfo->MaximumComponentNameLength;
                }

            if ( ARGUMENT_PRESENT(lpFileSystemFlags) ) {
                *lpFileSystemFlags = AttributeInfo->FileSystemAttributes;
                }
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            XapiSetLastNTError(STATUS_ACCESS_VIOLATION);
            return FALSE;
            }
        rv = TRUE;
finally_exit:;
        }
    finally {
        NtClose(Handle);
        if ( VolumeInfo ) {
            RtlFreeHeap(XapiProcessHeap, 0,VolumeInfo);
            }
        if ( AttributeInfo ) {
            RtlFreeHeap(XapiProcessHeap, 0,AttributeInfo);
            }
        }
    return rv;
}

NTSTATUS
XapiSelectCachePartition(
    IN BOOL fAlwaysFormat,
    OUT PULONG pnCachePartition,
    OUT PBOOL pfForceFormat
    )
{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS          Status;
    IO_STATUS_BLOCK   statusBlock;
    HANDLE            hVolume;
    DWORD             dwTitleId = XeImageHeader()->Certificate->TitleID;
    ULONG             CachePartitionCount;
    ULONG             nCachePartition;

    ASSERT(pnCachePartition && pfForceFormat);

    InitializeObjectAttributes(&oa,
                               (POBJECT_STRING) &XapiHardDisk,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&hVolume,
                        SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                        &oa,
                        &statusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    if (NT_SUCCESS(Status))
    {
        UCHAR rgbSectorBuffer[XBOX_HD_SECTOR_SIZE];
        LARGE_INTEGER byteOffset;

        //
        // Read sector 4 (XBOX_CACHE_DB_SECTOR_INDEX)
        //

        byteOffset.QuadPart = XBOX_CACHE_DB_SECTOR_INDEX * XBOX_HD_SECTOR_SIZE;

        Status = NtReadFile(hVolume,
                            0,
                            NULL,
                            NULL,
                            &statusBlock,
                            rgbSectorBuffer,
                            sizeof(rgbSectorBuffer),
                            &byteOffset);

        if (NT_SUCCESS(Status))
        {
            PXBOX_CACHE_DB_SECTOR pCacheDBSec = (PXBOX_CACHE_DB_SECTOR) rgbSectorBuffer;
            PX_CACHE_DB_ENTRY pCacheDB = (PX_CACHE_DB_ENTRY) pCacheDBSec->Data;
            ULONG i;
            ULONG iPrevDBIndex = (HalDiskCachePartitionCount - 1);
            ULONG iNewDBIndex;

            if ((XBOX_CACHE_DB_SECTOR_BEGIN_SIGNATURE != pCacheDBSec->SectorBeginSignature) ||
                (XBOX_CACHE_DB_SECTOR_END_SIGNATURE != pCacheDBSec->SectorEndSignature) ||
                (XBOX_CACHE_DB_CUR_VERSION != pCacheDBSec->Version))
            {
                RtlZeroMemory(rgbSectorBuffer, sizeof(rgbSectorBuffer));

                pCacheDBSec->SectorBeginSignature = XBOX_CACHE_DB_SECTOR_BEGIN_SIGNATURE;
                pCacheDBSec->Version = XBOX_CACHE_DB_CUR_VERSION;
                pCacheDBSec->SectorEndSignature = XBOX_CACHE_DB_SECTOR_END_SIGNATURE;
            }

            //
            // Assume that we're going to force the partition to be formatted unless
            // we find out otherwise
            //

            *pfForceFormat = TRUE;

            //
            // Obtain the number of cache partitions from the HAL.  The HAL
            // won't boot with a drive too small to contain one cache
            // partition, but we do need to limit the cache partition count
            // to the number that we can describe in the cache partition
            // database.
            //

            CachePartitionCount = HalDiskCachePartitionCount;

            RXDK_INIT_TRACE1("HalDiskCachePartitionCount=%u", CachePartitionCount);

            ASSERT(CachePartitionCount > 0);

            if (CachePartitionCount > XBOX_CACHE_DB_MAX_ENTRY_COUNT)
            {
                CachePartitionCount = XBOX_CACHE_DB_MAX_ENTRY_COUNT;
            }

            nCachePartition = 0;

            //
            // Search for a cache partition already allocated by this title
            //
            
            for (i = 0; i < CachePartitionCount; i++)
            {
                if ((dwTitleId == pCacheDB[i].dwTitleId) && pCacheDB[i].fUsed)
                {
                    nCachePartition = pCacheDB[i].nCacheIndex + XDISK_FIRST_CACHE_PARTITION;
                    iPrevDBIndex = i;
                    
                    //
                    // We found an existing cache partition, there is no longer a
                    // requirement that the partition be formatted.
                    //

                    *pfForceFormat = FALSE;
                    
                    break;
                }
            }

            //
            // If that search failed, search for a cache partition that is not in use
            // using ugly n-squared algorithm (fortunately, CachePartitionCount is small)
            //
            
            if (0 == nCachePartition)
            {
                UINT j;
                for (j = 0; j < CachePartitionCount; j++)
                {
                    for (i = 0; i < CachePartitionCount; i++)
                    {
                        if ((pCacheDB[i].fUsed) && (pCacheDB[i].nCacheIndex == j))
                        {
                            break;
                        }
                    }

                    //
                    // If we made it through the loop without a match, then this
                    // cache partition (index stored in the j variable) is available
                    //
                    
                    if (i == CachePartitionCount)
                    {
                        nCachePartition = j + XDISK_FIRST_CACHE_PARTITION;
                    }
                }
            }
                
            //
            // If that search failed, grab the oldest cache partition
            //
            // The Cache DB is stored in MRU order - the first entry was the most recently
            // used and the last entry was the least recently used
            //
            
            if (0 == nCachePartition)
            {
                nCachePartition = pCacheDB[CachePartitionCount - 1].nCacheIndex + XDISK_FIRST_CACHE_PARTITION;
            }

            //
            // If the value we've chosen is too large for some reason, pull it back and give
            // it a reasonable value
            //
            
            if (nCachePartition >= CachePartitionCount + XDISK_FIRST_CACHE_PARTITION)
            {
                nCachePartition = (CachePartitionCount - 1) + XDISK_FIRST_CACHE_PARTITION;
            }

            ASSERT(nCachePartition != 0);
            *pnCachePartition = nCachePartition;

            //
            // Normally, we bump this to the top of the cache db because that is how we
            // indicate that it was most recently used.  When fAlwaysFormat is set, we
            // always put it at the end of the list because we want it to be reclaimed
            // when the next title asks for a cache partition
            //
            
            iNewDBIndex = fAlwaysFormat ? (CachePartitionCount - 1) : 0;

            if (!fAlwaysFormat && (0 != iPrevDBIndex))
            {
                //
                // Modify the cache db - slide everything down and make room for this
                // entry at the top of the list
                //

                ASSERT(iPrevDBIndex < CachePartitionCount);
                
                RtlMoveMemory(&(pCacheDB[1]),
                              &(pCacheDB[0]),
                              iPrevDBIndex * sizeof(X_CACHE_DB_ENTRY));
            }
            
            //
            // Write this entry into the new index of the cache db
            //
            // Note that if this function was called with fAlwaysFormat set to TRUE,
            // the entry will be marked with fUsed == FALSE, so that it will be chosen
            // first the next time a title needs to allocate a new partition
            //

            pCacheDB[iNewDBIndex].dwTitleId = dwTitleId;
            pCacheDB[iNewDBIndex].nCacheIndex = (nCachePartition - XDISK_FIRST_CACHE_PARTITION);
            pCacheDB[iNewDBIndex].fUsed = (!fAlwaysFormat);

            //
            // Record which DB slot the Z: (primary) entry now lives in, so
            // XMountSecondaryUtilityDrive/XSwapUtilityDrives can find Z:'s
            // partition. This is the slot we just wrote above.
            //

            g_iZDriveDBIndex = iNewDBIndex;

            //
            // Ignore status result
            //

            NtWriteFile(hVolume,
                        0,
                        NULL,
                        NULL,
                        &statusBlock,
                        rgbSectorBuffer,
                        sizeof(rgbSectorBuffer),
                        &byteOffset);
        }

        NtClose(hVolume);
    }

    return Status;
}


BOOL
__attribute__((__stdcall__))
XMountUtilityDrive(
    BOOL fFormatClean
    )
{
    BOOL fRet = TRUE;
    BOOL fForceFormat;
    ULONG nPartition;
    NTSTATUS Status;

    RXDK_INIT_TRACE("XMountUtilityDrive enter");
#if DBG
    if (g_fMountedUtilityDrive)
    {
        RIP("XMountUtilityDrive(): Utility Drive has already been mounted");
    }
#endif // DBG

    Status = XapiSelectCachePartition(fFormatClean, &nPartition, &fForceFormat);
    RXDK_INIT_TRACE2("cache partition n=%u forceFmt=%u", nPartition, (unsigned)fForceFormat);

    if (NT_SUCCESS(Status))
    {
        OCHAR szCacheDrive[MAX_PATH];
        OBJECT_STRING VolString, DriveString;
        BOOL fDoFormat = (fFormatClean || fForceFormat);
        ULONG BytesPerCluster = XeUtilityDriveClusterSize();

        _snoprintf(szCacheDrive,
                   ARRAYSIZE(szCacheDrive),
                   CacheDriveFormat,
                   nPartition);

        RXDK_INIT_TRACE1("cache path %s", szCacheDrive);

        RtlInitObjectString(&VolString, szCacheDrive);

        //
        // The DriveString should not end in a backslash, so init from the same
        // string, but subtract a character on the Length member.
        //

        RtlInitObjectString(&DriveString, szCacheDrive);
        DriveString.Length -= sizeof(OCHAR);

        if (fDoFormat)
        {
            RXDK_INIT_TRACE("format utility");
            fRet = XapiFormatFATVolumeEx(&DriveString, BytesPerCluster);
        }

        if (fRet)
        {
            RXDK_INIT_TRACE("validate utility");
            Status = XapiValidateDiskPartitionEx(&VolString, BytesPerCluster);
            RXDK_INIT_TRACE1("validate utility status=%08x", (unsigned)Status);

            if (!NT_SUCCESS(Status) && !fDoFormat)
            {
                //
                // If the validate failed for some reason and we didn't just format
                // the partition, go ahead and format it now (make the system more
                // self-healing)
                //

                if (XapiFormatFATVolumeEx(&DriveString, BytesPerCluster))
                {
                    Status = XapiValidateDiskPartitionEx(&VolString, BytesPerCluster);
                }
            }

            if (NT_SUCCESS(Status))
            {
                RXDK_INIT_TRACE("Z: link");
                // Give the cache partition a drive letter
                Status = IoCreateSymbolicLink((POBJECT_STRING) &ZDrive, &DriveString);
            }

            fRet = NT_SUCCESS(Status);

            if (!fRet)
            {
                XapiSetLastNTError(Status);
            }
        }
    }
    else
    {
        fRet = FALSE;
        XapiSetLastNTError(Status);
    }

#if DBG
    if (fRet)
    {
        g_fMountedUtilityDrive = TRUE;
    }
#endif // DBG

    return fRet;
}


BOOL
__attribute__((__stdcall__))
XFormatUtilityDrive(
    VOID
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CHAR Target[MAX_PATH];
    ULONG TargetLength;
    OBJECT_STRING ObjectTarget;
    HANDLE Handle;

#if DBG
    if (!g_fMountedUtilityDrive)
    {
        RIP("XFormatUtilityDrive(): Utility Drive has not been mounted");
    }
#endif // DBG

    InitializeObjectAttributes(&ObjectAttributes,
                               (POBJECT_STRING) &ZDrive,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenSymbolicLinkObject(&Handle, &ObjectAttributes);

    if (!NT_SUCCESS(status))
    {
        XapiSetLastNTError(status);
        return FALSE;
    }

    ObjectTarget.Buffer = Target;
    ObjectTarget.MaximumLength = sizeof(Target);

    status = NtQuerySymbolicLinkObject(Handle, &ObjectTarget, &TargetLength);

    NtClose(Handle);

    if (!NT_SUCCESS(status))
    {
        XapiSetLastNTError(status);
        return FALSE;
    }

    return XapiFormatFATVolumeEx(&ObjectTarget, XeUtilityDriveClusterSize());
}


//
// XMountSecondaryUtilityDrive -- mount a SECOND cache partition as N:, in
// addition to the primary Z: mounted by XMountUtilityDrive.
//
// It reads the on-disk cache-partition database (sector XBOX_CACHE_DB_SECTOR_INDEX
// of \Device\Harddisk0\partition0), finds a free cache partition that is not Z:'s,
// records it in the least-recently-used DB slot, formats it, and symlinks \??\N:.
// The primary path (XapiSelectCachePartition) must have run first -- it sets
// g_iZDriveDBIndex, without which there is no Z: to pair a secondary against.
//
// ndriveindex, like retail's iPrevDBIndex, uses the RAW HAL partition count minus
// one (before the MAX_ENTRY_COUNT cap); the HAL guarantees a small count, so this
// never exceeds the table in practice.
//
BOOL
__attribute__((__stdcall__))
XMountSecondaryUtilityDrive(
    VOID
    )
{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS          Status;
    IO_STATUS_BLOCK   statusBlock;
    HANDLE            hVolume;
    DWORD             dwTitleId = XeImageHeader()->Certificate->TitleID;
    ULONG             nCachePartition = 0;    // chosen partition number (0 = none)
    CHAR              sz[MAX_PATH];
    OBJECT_STRING     VolString, DriveString;

    //
    // The primary utility drive must be mounted first -- we mirror its DB slot.
    //

    if (g_iZDriveDBIndex == (ULONG)-1)
    {
        XapiSetLastNTError(STATUS_UNSUCCESSFUL);
        return FALSE;
    }

    InitializeObjectAttributes(&oa,
                               (POBJECT_STRING) &XapiHardDisk,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&hVolume,
                        SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                        &oa,
                        &statusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    if (NT_SUCCESS(Status))
    {
        UCHAR rgbSectorBuffer[XBOX_HD_SECTOR_SIZE];
        LARGE_INTEGER byteOffset;

        byteOffset.QuadPart = XBOX_CACHE_DB_SECTOR_INDEX * XBOX_HD_SECTOR_SIZE;

        Status = NtReadFile(hVolume, 0, NULL, NULL, &statusBlock,
                            rgbSectorBuffer, sizeof(rgbSectorBuffer), &byteOffset);

        if (NT_SUCCESS(Status))
        {
            PXBOX_CACHE_DB_SECTOR pCacheDBSec = (PXBOX_CACHE_DB_SECTOR) rgbSectorBuffer;
            PX_CACHE_DB_ENTRY     pCacheDB    = (PX_CACHE_DB_ENTRY) pCacheDBSec->Data;
            ULONG iLastDBIndex = (HalDiskCachePartitionCount - 1);
            ULONG CachePartitionCount = HalDiskCachePartitionCount;

            if (CachePartitionCount > XBOX_CACHE_DB_MAX_ENTRY_COUNT)
            {
                CachePartitionCount = XBOX_CACHE_DB_MAX_ENTRY_COUNT;
            }

            //
            // Unlike XapiSelectCachePartition, the secondary path does not
            // reinitialise a bad database -- it refuses to proceed.
            //

            if ((XBOX_CACHE_DB_SECTOR_BEGIN_SIGNATURE != pCacheDBSec->SectorBeginSignature) ||
                (XBOX_CACHE_DB_SECTOR_END_SIGNATURE != pCacheDBSec->SectorEndSignature) ||
                (XBOX_CACHE_DB_CUR_VERSION != pCacheDBSec->Version))
            {
                Status = 0xC00000E4;    // internal DB-invalid NTSTATUS
            }
            else
            {
                ULONG zCacheIndex = pCacheDB[g_iZDriveDBIndex].nCacheIndex;
                ULONG nIndex;
                ULONG i, j;

                //
                // Find a free cache partition that is not Z:'s. The scan does not
                // break: nCachePartition ends as (highest free index) + base, or
                // 0 if none is free.
                //

                for (j = 0; j < CachePartitionCount; j++)
                {
                    if (j == zCacheIndex)
                        continue;

                    for (i = 0; i < CachePartitionCount; i++)
                    {
                        if (pCacheDB[i].fUsed && (pCacheDB[i].nCacheIndex == j))
                            break;
                    }

                    if (i == CachePartitionCount)
                        nCachePartition = j + XDISK_FIRST_CACHE_PARTITION;
                }

                //
                // N: takes the least-recently-used slot, avoiding Z:'s.
                //

                nIndex = iLastDBIndex;
                if (nIndex == g_iZDriveDBIndex)
                    nIndex--;
                g_iNDriveDBIndex = nIndex;

                if (nCachePartition != 0)
                {
                    pCacheDB[nIndex].nCacheIndex = nCachePartition - XDISK_FIRST_CACHE_PARTITION;
                }
                else
                {
                    //
                    // Nothing free -- reuse the partition already in this slot.
                    //
                    nCachePartition = pCacheDB[nIndex].nCacheIndex + XDISK_FIRST_CACHE_PARTITION;
                }

                pCacheDB[nIndex].dwTitleId = dwTitleId;
                pCacheDB[nIndex].fUsed = FALSE;     // reclaimable, like fAlwaysFormat

                NtWriteFile(hVolume, 0, NULL, NULL, &statusBlock,
                            rgbSectorBuffer, sizeof(rgbSectorBuffer), &byteOffset);
                Status = STATUS_SUCCESS;
            }
        }

        NtClose(hVolume);
    }

    //
    // A read/validation failure, or no partition chosen, is a hard failure.
    //

    if ((nCachePartition == 0) || ((Status & 0xC0000000) == 0xC0000000))
    {
        g_iNDriveDBIndex = (ULONG)-1;
        if ((Status & 0xC0000000) == 0xC0000000)
            XapiSetLastNTError(Status);
        return FALSE;
    }

    //
    // Format the chosen partition and link it as N:.
    //

    _snprintf(sz, sizeof(sz), CacheDriveFormat, nCachePartition);

    RtlInitAnsiString(&VolString, sz);      // keeps the trailing backslash
    RtlInitAnsiString(&DriveString, sz);
    DriveString.Length -= sizeof(OCHAR);    // strip the trailing backslash

    if (!XapiFormatFATVolumeEx(&DriveString, XeUtilityDriveClusterSize()))
    {
        //
        // The format-failure path does NOT set the last error (matches retail).
        //
        g_iNDriveDBIndex = (ULONG)-1;
        return FALSE;
    }

    Status = XapiValidateDiskPartitionEx(&VolString, XeUtilityDriveClusterSize());
    if (NT_SUCCESS(Status))
    {
        Status = IoCreateSymbolicLink((POBJECT_STRING) &NDrive, &DriveString);
    }

    if ((Status & 0xC0000000) == 0xC0000000)
    {
        g_iNDriveDBIndex = (ULONG)-1;
        XapiSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


//
// XSwapUtilityDrives -- exchange which physical partition Z: and N: point at.
// It swaps ONLY the two DB records' nCacheIndex fields (ownership metadata stays
// put), persists the sector,
// then re-points the \??\Z: and \??\N: symlinks. The DB slot globals are unchanged.
//
BOOL
__attribute__((__stdcall__))
XSwapUtilityDrives(
    VOID
    )
{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS          Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK   statusBlock;
    HANDLE            hVolume;
    ULONG             newNPartition = 0, newZPartition = 0;
    CHAR              sz[MAX_PATH];
    OBJECT_STRING     DriveString;

    if ((g_iZDriveDBIndex == (ULONG)-1) || (g_iNDriveDBIndex == (ULONG)-1))
    {
        XapiSetLastNTError(STATUS_UNSUCCESSFUL);
        return FALSE;
    }

    InitializeObjectAttributes(&oa,
                               (POBJECT_STRING) &XapiHardDisk,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&hVolume,
                        SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                        &oa,
                        &statusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    if (NT_SUCCESS(Status))
    {
        UCHAR rgbSectorBuffer[XBOX_HD_SECTOR_SIZE];
        LARGE_INTEGER byteOffset;

        byteOffset.QuadPart = XBOX_CACHE_DB_SECTOR_INDEX * XBOX_HD_SECTOR_SIZE;

        Status = NtReadFile(hVolume, 0, NULL, NULL, &statusBlock,
                            rgbSectorBuffer, sizeof(rgbSectorBuffer), &byteOffset);

        if (NT_SUCCESS(Status))
        {
            PXBOX_CACHE_DB_SECTOR pCacheDBSec = (PXBOX_CACHE_DB_SECTOR) rgbSectorBuffer;
            PX_CACHE_DB_ENTRY     pCacheDB    = (PX_CACHE_DB_ENTRY) pCacheDBSec->Data;

            if ((XBOX_CACHE_DB_SECTOR_BEGIN_SIGNATURE != pCacheDBSec->SectorBeginSignature) ||
                (XBOX_CACHE_DB_SECTOR_END_SIGNATURE != pCacheDBSec->SectorEndSignature) ||
                (XBOX_CACHE_DB_CUR_VERSION != pCacheDBSec->Version))
            {
                Status = 0xC00000E4;
            }
            else
            {
                //
                // Swap only the partition assignments of the two records.
                //
                ULONG tmp = pCacheDB[g_iNDriveDBIndex].nCacheIndex;
                pCacheDB[g_iNDriveDBIndex].nCacheIndex = pCacheDB[g_iZDriveDBIndex].nCacheIndex;
                pCacheDB[g_iZDriveDBIndex].nCacheIndex = tmp;

                newNPartition = pCacheDB[g_iNDriveDBIndex].nCacheIndex + XDISK_FIRST_CACHE_PARTITION;
                newZPartition = pCacheDB[g_iZDriveDBIndex].nCacheIndex + XDISK_FIRST_CACHE_PARTITION;

                Status = NtWriteFile(hVolume, 0, NULL, NULL, &statusBlock,
                                     rgbSectorBuffer, sizeof(rgbSectorBuffer), &byteOffset);
            }
        }

        NtClose(hVolume);
    }

    if (NT_SUCCESS(Status))
        Status = IoDeleteSymbolicLink((POBJECT_STRING) &NDrive);
    if (NT_SUCCESS(Status))
        Status = IoDeleteSymbolicLink((POBJECT_STRING) &ZDrive);

    if (NT_SUCCESS(Status))
    {
        _snprintf(sz, sizeof(sz), CacheDriveFormat, newNPartition);
        RtlInitAnsiString(&DriveString, sz);
        DriveString.Length -= sizeof(OCHAR);
        Status = IoCreateSymbolicLink((POBJECT_STRING) &NDrive, &DriveString);
    }
    if (NT_SUCCESS(Status))
    {
        _snprintf(sz, sizeof(sz), CacheDriveFormat, newZPartition);
        RtlInitAnsiString(&DriveString, sz);
        DriveString.Length -= sizeof(OCHAR);
        Status = IoCreateSymbolicLink((POBJECT_STRING) &ZDrive, &DriveString);
    }

    if ((Status & 0xC0000000) == 0xC0000000)
    {
        XapiSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


//
// XFormatSecondaryUtilityDrive -- reformat the N: partition in place. Identical
// in shape to XFormatUtilityDrive, only the symbolic link resolved differs. It
// touches neither the database nor g_iNDriveDBIndex.
//
BOOL
__attribute__((__stdcall__))
XFormatSecondaryUtilityDrive(
    VOID
    )
{
    NTSTATUS          status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CHAR              Target[MAX_PATH];
    ULONG             TargetLength;
    OBJECT_STRING     ObjectTarget;
    HANDLE            Handle;

    InitializeObjectAttributes(&ObjectAttributes,
                               (POBJECT_STRING) &NDrive,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenSymbolicLinkObject(&Handle, &ObjectAttributes);

    if (!NT_SUCCESS(status))
    {
        XapiSetLastNTError(status);
        return FALSE;
    }

    ObjectTarget.Buffer = Target;
    ObjectTarget.MaximumLength = sizeof(Target);

    status = NtQuerySymbolicLinkObject(Handle, &ObjectTarget, &TargetLength);

    NtClose(Handle);

    if (!NT_SUCCESS(status))
    {
        XapiSetLastNTError(status);
        return FALSE;
    }

    return XapiFormatFATVolumeEx(&ObjectTarget, XeUtilityDriveClusterSize());
}


DWORD
__attribute__((__stdcall__))
XMountAlternateTitle(
    IN PCOSTR lpRootPath,
    IN DWORD dwAltTitleId,
    OUT POCHAR pchDrive
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_STRING ObjectName;
    OCHAR szDosDevice[MAX_PATH];
    OCHAR Target[MAX_PATH];
    ULONG TargetLength;
    OBJECT_STRING ObjectTarget;
    OCHAR AltTitleId[CCHMAX_HEX_DWORD];
    HANDLE Handle;
    OCHAR chDrive;
    BOOL fTData;
    PXBEIMAGE_CERTIFICATE Certificate = XeImageHeader()->Certificate;
    int i;

    RIP_ON_NOT_TRUE(XMountAlternateTitle, (lpRootPath != NULL));
    RIP_ON_NOT_TRUE(XMountAlternateTitle, (lpRootPath[0] != '\0'));
    RIP_ON_NOT_TRUE(XMountAlternateTitle, (lpRootPath[1] == ':'));
    RIP_ON_NOT_TRUE(XMountAlternateTitle, (lpRootPath[2] == '\\'));
    RIP_ON_NOT_TRUE(XMountAlternateTitle, (lpRootPath[3] == '\0'));
    RIP_ON_NOT_TRUE(XMountAlternateTitle, (pchDrive != NULL));

    //
    // Removing the 0x20 bit will make lower case characters uppercase
    //

    chDrive = lpRootPath[0] & (~0x20);
    fTData = (HD_TDATA_DRIVE == chDrive);

#if DBG
    if (((chDrive < MU_FIRST_DRIVE) || (chDrive > MU_LAST_DRIVE)) &&
        (HD_UDATA_DRIVE != chDrive) &&
        (!fTData))
    {
        RIP("XFindFirstSaveGame() invalid drive letter parameter");
    }
#endif // DBG

    for (i = 0; i < ARRAYSIZE(Certificate->AlternateTitleIDs); i++)
    {
        if (0 == Certificate->AlternateTitleIDs[i])
        {
            return ERROR_ACCESS_DENIED;
        }

        if (dwAltTitleId == Certificate->AlternateTitleIDs[i])
        {
            break;
        }
    }

    if (i >= sizeof(Certificate->AlternateTitleIDs))
    {
        return ERROR_ACCESS_DENIED;
    }

    soprintf(szDosDevice, OTEXT("\\??\\%c:"), lpRootPath[0]);

    RtlInitObjectString(&ObjectName, szDosDevice);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ObjectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenSymbolicLinkObject(&Handle, &ObjectAttributes);

    if (!NT_SUCCESS(status))
    {
        return RtlNtStatusToDosError(status);
    }

    ObjectTarget.Buffer = Target;
    ObjectTarget.MaximumLength = sizeof(Target);

    status = NtQuerySymbolicLinkObject(Handle, &ObjectTarget, &TargetLength);

    NtClose(Handle);

    if (!NT_SUCCESS(status))
    {
        return RtlNtStatusToDosError(status);
    }

    if ((TargetLength < CCHMAX_HEX_DWORD) ||
        ('\\' != Target[TargetLength - (CCHMAX_HEX_DWORD)]))
    {
        return ERROR_INVALID_DRIVE;
    }

    //
    // Remove the existing title id from the end of the string so we can
    // reuse ObjectTarget below in XapiMapLetterToDirectory()
    //

    ObjectTarget.Length -= CCHMAX_HEX_DWORD;

    DwordToStringO(dwAltTitleId, AltTitleId);

    status = XapiMapLetterToDirectory(fTData ? &WDrive : &XDrive,
                                      (PCOBJECT_STRING) &ObjectTarget,
                                      AltTitleId,
                                      FALSE,
                                      NULL,
                                      FALSE);

    if (NT_SUCCESS(status))
    {
        *pchDrive = fTData ? HD_ALT_TDATA_DRIVE : HD_ALT_UDATA_DRIVE;

        if (!fTData && (HD_UDATA_DRIVE != chDrive))
        {
            //
            // Remember that we've mapped an alternate drive letter to this MU
            // drive so that we can unmount the alternate drive automatically
            // if the "real" MU drive is unmounted later using XUnmountMU()
            //

            ASSERT(OBJECT_NULL == XapiMuInfo.DriveWithAltDriveMapped);
            XapiMuInfo.DriveWithAltDriveMapped = chDrive;
        }
    }

    return RtlNtStatusToDosError(status);
}

DWORD
__attribute__((__stdcall__))
XUnmountAlternateTitle(
    IN OCHAR chDrive
    )
{
    NTSTATUS Status;
    OCHAR szDosDevice[MAX_PATH];
    OBJECT_STRING DosDevice;

    //
    // Removing the 0x20 bit will make lower case characters uppercase
    //

    chDrive &= (~0x20);

#if DBG
    switch (chDrive)
    {
        case HD_ALT_TDATA_DRIVE:
        case HD_ALT_UDATA_DRIVE:
            break;

        default:
            RIP("XUnmountAlternateTitle() - invalid chDrive parameter");
    }
#endif // DBG

    soprintf(szDosDevice, OTEXT("\\??\\%c:"), chDrive);

    RtlInitObjectString(&DosDevice, szDosDevice);

    //
    // BUGBUG: Do more than remove the symbolic link - we need to unmount
    // the filesystem here.
    //

    Status = IoDeleteSymbolicLink(&DosDevice);

    if ((HD_ALT_UDATA_DRIVE == chDrive) && NT_SUCCESS(Status))
    {
        XapiMuInfo.DriveWithAltDriveMapped = OBJECT_NULL;
    }

    return RtlNtStatusToDosError(Status);
}

DWORD
__attribute__((__stdcall__))
XGetDiskClusterSize(
    PCOSTR lpRootPathName
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    OBJECT_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION NormalSizeInfo;

    RIP_ON_NOT_TRUE("XGetDiskClusterSize()", ARGUMENT_PRESENT(lpRootPathName));

    RtlInitObjectString(&FileName, lpRootPathName);

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        ObDosDevicesDirectory(),
        NULL
        );

    //
    // Open the file
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_FREE_SPACE_QUERY
                );
    if ( !NT_SUCCESS(Status) ) {
        XapiSetLastNTError(Status);
        if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            }
        return 0;
        }

    //
    // Determine the size parameters of the volume.
    //

    Status = NtQueryVolumeInformationFile(
                Handle,
                &IoStatusBlock,
                &NormalSizeInfo,
                sizeof(NormalSizeInfo),
                FileFsSizeInformation
                );
    NtClose(Handle);
    if ( !NT_SUCCESS(Status) ) {
        XapiSetLastNTError(Status);
        return 0;
        }

    ASSERT((0 != NormalSizeInfo.BytesPerSector) && (0 != NormalSizeInfo.SectorsPerAllocationUnit));

    return (ULONG) (NormalSizeInfo.BytesPerSector * NormalSizeInfo.SectorsPerAllocationUnit);
}

DWORD
__attribute__((__stdcall__))
XGetDiskSectorSize(
    PCOSTR lpRootPathName
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    OBJECT_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION NormalSizeInfo;

    RIP_ON_NOT_TRUE("XGetDiskSectorSize()", ARGUMENT_PRESENT(lpRootPathName));

    RtlInitObjectString(&FileName, lpRootPathName);

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        ObDosDevicesDirectory(),
        NULL
        );

    //
    // Open the file
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_FREE_SPACE_QUERY
                );
    if ( !NT_SUCCESS(Status) ) {
        XapiSetLastNTError(Status);
        if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            }
        return 0;
        }

    //
    // Determine the size parameters of the volume.
    //

    Status = NtQueryVolumeInformationFile(
                Handle,
                &IoStatusBlock,
                &NormalSizeInfo,
                sizeof(NormalSizeInfo),
                FileFsSizeInformation
                );
    NtClose(Handle);
    if ( !NT_SUCCESS(Status) ) {
        XapiSetLastNTError(Status);
        return 0;
        }

    ASSERT(0 != NormalSizeInfo.BytesPerSector);

    return NormalSizeInfo.BytesPerSector;
}


DWORD
__attribute__((__stdcall__))
XMUNameFromDriveLetter(
    IN CHAR chDrive,
    OUT LPWSTR lpName,
    IN UINT cchName
    )
{
    NTSTATUS Status;
    OCHAR szDosDevice[8];
    OBJECT_STRING DosDevice;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    FSCTL_VOLUME_METADATA VolumeMetadata;
    WCHAR VolumeName[FAT_VOLUME_NAME_LENGTH];

    //
    // Fail if the device is not already mounted.
    //
    if (!MU_IS_MOUNTED(chDrive))
    {
        XDBGERR("XAPI", "XMUNameFromDriveLetter() MU %c: is not mounted", chDrive);
        return ERROR_INVALID_DRIVE;
    }

    //
    // Open a handle to the volume or directory of the drive.
    //
    soprintf(szDosDevice, OTEXT("\\??\\%c:"), chDrive);
    RtlInitObjectString(&DosDevice, szDosDevice);

    InitializeObjectAttributes(
        &Obja,
        (POBJECT_STRING) &DosDevice,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(&Handle,
                        SYNCHRONIZE | GENERIC_READ,
                        &Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    if (NT_SUCCESS(Status))
    {
        VolumeMetadata.ByteOffset = FIELD_OFFSET(FAT_VOLUME_METADATA, VolumeName);
        VolumeMetadata.TransferLength = sizeof(VolumeName);
        VolumeMetadata.TransferBuffer = VolumeName;

        Status = NtFsControlFile(Handle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_READ_VOLUME_METADATA,
                                 &VolumeMetadata,
                                 sizeof(VolumeMetadata),
                                 NULL,
                                 0);

        if (NT_SUCCESS(Status))
        {
            lstrcpynW(lpName, VolumeName, min(cchName, FAT_VOLUME_NAME_LENGTH));
        }

        NtClose(Handle);
    }

    return RtlNtStatusToDosError(Status);
}

#endif // ! XAPILIBP

#ifdef XAPILIBP

VOID
XapiDeleteCachePartition(
    IN DWORD dwTitleId
    )
{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS          Status;
    IO_STATUS_BLOCK   statusBlock;
    HANDLE            hVolume;
    ULONG             CachePartitionCount;

    InitializeObjectAttributes(&oa,
                               (POBJECT_STRING) &XapiHardDisk,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&hVolume,
                        SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                        &oa,
                        &statusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    if (NT_SUCCESS(Status))
    {
        UCHAR rgbSectorBuffer[XBOX_HD_SECTOR_SIZE];
        LARGE_INTEGER byteOffset;

        //
        // Read sector 4 (XBOX_CACHE_DB_SECTOR_INDEX)
        //

        byteOffset.QuadPart = XBOX_CACHE_DB_SECTOR_INDEX * XBOX_HD_SECTOR_SIZE;

        Status = NtReadFile(hVolume,
                            0,
                            NULL,
                            NULL,
                            &statusBlock,
                            rgbSectorBuffer,
                            sizeof(rgbSectorBuffer),
                            &byteOffset);

        if (NT_SUCCESS(Status))
        {
            PXBOX_CACHE_DB_SECTOR pCacheDBSec = (PXBOX_CACHE_DB_SECTOR) rgbSectorBuffer;
            PX_CACHE_DB_ENTRY pCacheDB = (PX_CACHE_DB_ENTRY) pCacheDBSec->Data;
            ULONG i;

            if ((XBOX_CACHE_DB_SECTOR_BEGIN_SIGNATURE != pCacheDBSec->SectorBeginSignature) ||
                (XBOX_CACHE_DB_SECTOR_END_SIGNATURE != pCacheDBSec->SectorEndSignature) ||
                (XBOX_CACHE_DB_CUR_VERSION != pCacheDBSec->Version))
            {
                NtClose(hVolume);

                return;
            }

            //
            // Obtain the number of cache partitions from the HAL.  The HAL
            // won't boot with a drive too small to contain one cache
            // partition, but we do need to limit the cache partition count
            // to the number that we can describe in the cache partition
            // database.
            //

            CachePartitionCount = HalDiskCachePartitionCount;

            ASSERT(CachePartitionCount > 0);

            if (CachePartitionCount > XBOX_CACHE_DB_MAX_ENTRY_COUNT)
            {
                CachePartitionCount = XBOX_CACHE_DB_MAX_ENTRY_COUNT;
            }

            //
            // Search the cache partition database for a matching title ID
            // If found, clear the title ID so we can write it back to the sector
            //

            for (i = 0; i < CachePartitionCount; i++)
            {
                if (dwTitleId == pCacheDB[i].dwTitleId)
                {
                    pCacheDB[i].dwTitleId = 0;
                    pCacheDB[i].fUsed = FALSE;

                    break;
                }
            }

            //
            // If we picked this partition because it is the oldest and not because
            // we matched a TitleId, then we must format the cache partition before
            // giving it to the title
            //

            //
            // Write back into the cache db
            //

            //
            // Ignore status result
            //

            Status = NtWriteFile(hVolume,
                                 0,
                                 NULL,
                                 NULL,
                                 &statusBlock,
                                 rgbSectorBuffer,
                                 sizeof(rgbSectorBuffer),
                                 &byteOffset);
        }

        NtClose(hVolume);
    }
}


NTSTATUS
XapiGetCachePartitions(
    IN PX_CACHE_DB_ENTRY pCacheEntriesBuffer,
    IN UINT cbBufferSize,
    OUT PDWORD pdwNumCacheEntries )
{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS          Status;
    IO_STATUS_BLOCK   statusBlock;
    HANDLE            hVolume;
    ULONG             CachePartitionCount;

    ASSERT(pdwNumCacheEntries);

    //
    // Set the number of entries written to 0, in case of failure
    //

    *pdwNumCacheEntries = 0;

    InitializeObjectAttributes(&oa,
                               (POBJECT_STRING) &XapiHardDisk,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&hVolume,
                        SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                        &oa,
                        &statusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    if (NT_SUCCESS(Status))
    {
        UCHAR rgbSectorBuffer[XBOX_HD_SECTOR_SIZE];
        LARGE_INTEGER byteOffset;

        //
        // Read sector 4 (XBOX_CACHE_DB_SECTOR_INDEX)
        //

        byteOffset.QuadPart = XBOX_CACHE_DB_SECTOR_INDEX * XBOX_HD_SECTOR_SIZE;

        Status = NtReadFile(hVolume,
                            0,
                            NULL,
                            NULL,
                            &statusBlock,
                            rgbSectorBuffer,
                            sizeof(rgbSectorBuffer),
                            &byteOffset);

        if (NT_SUCCESS(Status))
        {
            PXBOX_CACHE_DB_SECTOR pCacheDBSec = (PXBOX_CACHE_DB_SECTOR) rgbSectorBuffer;
            PX_CACHE_DB_ENTRY pCacheDB = (PX_CACHE_DB_ENTRY) pCacheDBSec->Data;
            ULONG i;

            if ((XBOX_CACHE_DB_SECTOR_BEGIN_SIGNATURE != pCacheDBSec->SectorBeginSignature) ||
                (XBOX_CACHE_DB_SECTOR_END_SIGNATURE != pCacheDBSec->SectorEndSignature) ||
                (XBOX_CACHE_DB_CUR_VERSION != pCacheDBSec->Version))
            {
                RtlZeroMemory(rgbSectorBuffer, sizeof(rgbSectorBuffer));
            }

            //
            // Obtain the number of cache partitions from the HAL.  The HAL
            // won't boot with a drive too small to contain one cache
            // partition, but we do need to limit the cache partition count
            // to the number that we can describe in the cache partition
            // database.
            //

            CachePartitionCount = HalDiskCachePartitionCount;

            ASSERT(CachePartitionCount > 0);

            if (CachePartitionCount > XBOX_CACHE_DB_MAX_ENTRY_COUNT)
            {
                CachePartitionCount = XBOX_CACHE_DB_MAX_ENTRY_COUNT;
            }

            //
            // Search the cache partition database for a matching title ID
            //

            for (i = 0; i < CachePartitionCount; i++)
            {
                if( 0 != pCacheDB[i].dwTitleId )
                {
                    if( ( pCacheEntriesBuffer != NULL ) && ( ( sizeof( X_CACHE_DB_ENTRY ) * (*pdwNumCacheEntries + 1) ) <= cbBufferSize ) )
                    {
                        RtlCopyMemory(&(pCacheEntriesBuffer[*pdwNumCacheEntries]),
                                      &(pCacheDB[i]),
                                      sizeof(X_CACHE_DB_ENTRY));
                    }

                    *pdwNumCacheEntries += 1;
                }
            }
        }

        NtClose(hVolume);
    }

    return Status;
}


DWORD
__attribute__((__stdcall__))
XMUWriteNameToDriveLetter(
    IN CHAR chDrive,
    IN LPCWSTR lpName
    )
{
    NTSTATUS Status;
    OCHAR szDosDevice[8];
    OBJECT_STRING DosDevice;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    FSCTL_VOLUME_METADATA VolumeMetadata;
    WCHAR VolumeName[FAT_VOLUME_NAME_LENGTH];

    //
    //  Fail if the device is not already mounted.
    //
    if (!MU_IS_MOUNTED(chDrive))
    {
        XDBGERR("XAPI", "XMUNameFromDriveLetter() MU %c: is not mounted", chDrive);
        return ERROR_INVALID_DRIVE;
    }

    ASSERT(wcslen(lpName) < MAX_MUNAME);

    //
    // Open a handle to the volume or directory of the drive.
    //
    soprintf(szDosDevice, OTEXT("\\??\\%c:"), chDrive);
    RtlInitObjectString(&DosDevice, szDosDevice);

    InitializeObjectAttributes(
        &Obja,
        (POBJECT_STRING) &DosDevice,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(&Handle,
                        SYNCHRONIZE | GENERIC_WRITE,
                        &Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    if (NT_SUCCESS(Status))
    {
        lstrcpynW(VolumeName, lpName, FAT_VOLUME_NAME_LENGTH);

        VolumeMetadata.ByteOffset = FIELD_OFFSET(FAT_VOLUME_METADATA, VolumeName);
        VolumeMetadata.TransferLength = sizeof(VolumeName);
        VolumeMetadata.TransferBuffer = VolumeName;

        Status = NtFsControlFile(Handle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_WRITE_VOLUME_METADATA,
                                 &VolumeMetadata,
                                 sizeof(VolumeMetadata),
                                 NULL,
                                 0);

        NtClose(Handle);
    }

    return RtlNtStatusToDosError(Status);
}

#endif // XAPILIBP
