#include "bridge_k32.h"
/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Win32 high-resolution performance-counter APIs: QueryPerformanceCounter and
 * QueryPerformanceFrequency, implemented as hand-written assembly reading the
 * kernel time-stamp counter.
 */

#include "basedll.h"

__declspec(naked)
BOOL
__stdcall
QueryPerformanceCounter(
    LARGE_INTEGER *lpPerformanceCount
    )
/*++

    QueryPerformanceCounter -   provides access to a high-resolution
                                counter; frequency of this counter
                                is supplied by QueryPerformanceFrequency

        Inputs:

            lpPerformanceCount  -   a pointer to variable which
                                    will receive the counter

        Outputs:

            lpPerformanceCount  -   the current value of the counter,
                                    or 0 if it is not available

        Returns:

            TRUE if the performance counter is supported by the
            hardware, or FALSE if the performance counter is not
            supported by the hardware.


--*/
{
    __asm {
        mov     ecx, DWORD PTR [esp+4]  // ecx = lpPerformanceCount
        rdtsc
        mov     DWORD PTR [ecx], eax
        mov     DWORD PTR [ecx+4], edx
        xor     eax, eax
        inc     eax                     // return TRUE;
        ret     4
    }
}

__declspec(naked)
BOOL
__stdcall
QueryPerformanceFrequency(
    LARGE_INTEGER *lpFrequency
    )
/*++

    QueryPerformanceFrequency -   provides the frequency of the high-
                                  resolution counter returned by
                                  QueryPerformanceCounter

        Inputs:

            lpFrequency         -   a pointer to variable which
                                    will receive the frequency

        Outputs:

            lpPerformanceCount  -   the frequency of the counter,
                                    or 0 if it is not available

        Returns:

            TRUE if the performance counter is supported by the
            hardware, or FALSE if the performance counter is not
            supported by the hardware.

--*/
{
    __asm {
        mov     ecx, DWORD PTR [esp+4]
        mov     eax, 02BCBAB1Dh           // 733333333
        mov     DWORD PTR [ecx], eax
        mov     DWORD PTR [ecx+4], 0
        mov     eax, 1
        ret     4
    }
}
