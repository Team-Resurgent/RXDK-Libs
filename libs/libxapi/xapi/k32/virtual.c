#include "bridge_k32.h"
/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    virtual.c

Abstract:

    This module implements the Win32 virtual memory management services.

--*/

#include "basedll.h"
#pragma hdrstop


PVOID
__attribute__((__stdcall__))
VirtualAlloc(
    PVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect
    )
{
    NTSTATUS Status;

#if DBG
    if (lpAddress != NULL && (ULONG_PTR)lpAddress < MM_ALLOCATION_GRANULARITY)
    {
        RIP("VirtualAlloc() invalid parameter (lpAddress)");
    }
#endif // DBG

    Status = NtAllocateVirtualMemory( &lpAddress,
                                      0,
                                      &dwSize,
                                      flAllocationType,
                                      flProtect
                                    );

    if (NT_SUCCESS( Status )) {
        return( lpAddress );
        }
    else {
        XapiSetLastNTError( Status );
        return( NULL );
        }
}

BOOL
__attribute__((__stdcall__))
VirtualFree(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD dwFreeType
    )
{
    NTSTATUS Status;

    if ( (dwFreeType & MEM_RELEASE ) && dwSize != 0 ) {
        XapiSetLastNTError( STATUS_INVALID_PARAMETER );
        return FALSE;
        }

    Status = NtFreeVirtualMemory( &lpAddress,
                                  &dwSize,
                                  dwFreeType
                                );

    if (NT_SUCCESS( Status )) {
        return( TRUE );
        }
    else {
        XapiSetLastNTError( Status );
        return( FALSE );
        }
}


BOOL
__attribute__((__stdcall__))
VirtualProtect(
    PVOID lpAddress,
    SIZE_T dwSize,
    DWORD flNewProtect,
    PDWORD lpflOldProtect
    )
{
    NTSTATUS Status;

    Status = NtProtectVirtualMemory( &lpAddress,
                                     &dwSize,
                                     flNewProtect,
                                     lpflOldProtect
                                   );

    if (NT_SUCCESS( Status )) {
        return( TRUE );
        }
    else {
        XapiSetLastNTError( Status );
        return( FALSE );
        }
}

DWORD
__attribute__((__stdcall__))
VirtualQuery(
    LPCVOID lpAddress,
    PMEMORY_BASIC_INFORMATION lpBuffer,
    DWORD dwLength
    )
{
    NTSTATUS Status;

    Status = NtQueryVirtualMemory( (LPVOID)lpAddress,
                                   lpBuffer
                                 );
    if (NT_SUCCESS( Status )) {
        return( sizeof(*lpBuffer) );
        }
    else {
        XapiSetLastNTError( Status );
        return( 0 );
        }
}

PVOID
__attribute__((__stdcall__))
VirtualAllocEx(
    HANDLE hProcess,
    PVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect
    )
{

    return VirtualAlloc(
                lpAddress,
                dwSize,
                flAllocationType,
                flProtect
                );

}


BOOL
__attribute__((__stdcall__))
VirtualFreeEx(
    HANDLE hProcess,
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD dwFreeType
    )
{
    return VirtualFree(lpAddress,dwSize,dwFreeType);
}


BOOL
__attribute__((__stdcall__))
VirtualProtectEx(
    HANDLE hProcess,
    PVOID lpAddress,
    SIZE_T dwSize,
    DWORD flNewProtect,
    PDWORD lpflOldProtect
    )
{

    return VirtualProtect( lpAddress,
                           dwSize,
                           flNewProtect,
                           lpflOldProtect
                         );
}


DWORD
__attribute__((__stdcall__))
VirtualQueryEx(
    HANDLE hProcess,
    LPCVOID lpAddress,
    PMEMORY_BASIC_INFORMATION lpBuffer,
    DWORD dwLength
    )
{

    return VirtualQuery( lpAddress,
                         (PMEMORY_BASIC_INFORMATION)lpBuffer,
                         dwLength
                       );
}


VOID
__attribute__((__stdcall__))
GlobalMemoryStatus(
    LPMEMORYSTATUS lpBuffer
    )
{
    MM_STATISTICS MemoryStatistics;

    MemoryStatistics.Length = sizeof(MM_STATISTICS);
    MmQueryStatistics(&MemoryStatistics);

    lpBuffer->dwLength = sizeof(*lpBuffer);
    lpBuffer->dwMemoryLoad = 0;
    lpBuffer->dwTotalPageFile = 0;
    lpBuffer->dwAvailPageFile = 0;
    lpBuffer->dwTotalPhys = (MemoryStatistics.TotalPhysicalPages << PAGE_SHIFT);
    lpBuffer->dwAvailPhys = (MemoryStatistics.AvailablePages << PAGE_SHIFT);
    lpBuffer->dwTotalVirtual = (ULONG_PTR)MM_HIGHEST_USER_ADDRESS -
        (ULONG_PTR)MM_LOWEST_USER_ADDRESS + 1;
    lpBuffer->dwAvailVirtual = lpBuffer->dwTotalVirtual -
        MemoryStatistics.VirtualMemoryBytesReserved;
}
