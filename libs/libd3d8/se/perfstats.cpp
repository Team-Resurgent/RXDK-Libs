/*==========================================================================
 *
 *  File:       perfstats.cpp
 *  Content:    The D3DPERF_* statistics surface (d3d8perf.h).
 *
 *  RXDK 5849 uplift.
 *
 *  The XDK gathered these counters in a SEPARATE instrumented build of D3D
 *  (d3d8i.lib / d3d8d.lib). Retail d3d8.lib does not carry the instrumentation,
 *  because counting draw calls and state changes on every call is exactly the
 *  cost a shipping title does not want. RXDK builds one libd3d8, and it is the
 *  retail one.
 *
 *  So this file is deliberately partial, and which half is which matters:
 *
 *    REAL -- D3DPERF_GetPushBufferInfo. Every field of it is state the pusher
 *    already keeps, so a title profiling push-buffer pressure gets true
 *    numbers. Same for the reset timestamp and the put-pointer snapshot.
 *
 *    ZERO -- every event counter in D3DPERF (vertices, waits, state changes,
 *    ...) and the whole GPU busy/idle profile. Nothing increments them, because
 *    nothing is instrumented. A title reading them sees zeros, NOT small or
 *    approximate numbers, which is the honest signal that the measurement did
 *    not happen. The debug print in D3DPERF_GetStatistics says so once, so it
 *    cannot be mistaken for a title that simply drew nothing.
 *
 *  The statistics block is ~210 KB (m_ProfileData alone is WORD[105000]), which
 *  is why it lives here alone: the linker only pulls this member in for a title
 *  that actually calls one of these, so everyone else pays nothing.
 *
 *==========================================================================*/

#include "precomp.hpp"

// d3d8perf.h is only pulled by stats.hpp when the instrumented build is on, and
// this is the retail one -- so bring in the types directly.
#include "d3d8perf.h"

// CDevice / g_pDevice live in namespace D3D; the entry points must not.
using namespace D3D;

extern "C" {

// The statically-allocated statistics block D3DPERF_GetStatistics hands out.
// Zero-initialized, and it stays that way apart from what Reset records.
static D3DPERF g_D3DPerf;
static BOOL    g_fWarnedNotInstrumented = FALSE;

//
// Read the CPU time-stamp counter. The struct's timestamps are documented as
// TSC values, not milliseconds, so give it what it asks for.
//
static ULONGLONG D3DPERF_ReadTSC(void)
{
    ULONGLONG tsc;
    __asm
    {
        rdtsc
        mov     dword ptr [tsc], eax
        mov     dword ptr [tsc + 4], edx
    }
    return tsc;
}

void WINAPI D3DPERF_Reset()
{
    // Everything the instrumented build would have accumulated goes back to
    // zero; here that is the whole struct.
    memset(&g_D3DPerf, 0, sizeof(g_D3DPerf));

    g_D3DPerf.m_TSCLastResetVal = D3DPERF_ReadTSC();

    // Where the push buffer stood at reset -- real, and what makes
    // "dwords written since the last reset" meaningful to a caller that
    // compares it against GetPushBufferInfo.
    if (g_pDevice != NULL)
    {
        g_D3DPerf.m_pPutLocationAtReset = (DWORD *)g_pDevice->m_Pusher.m_pPut;
    }
}

D3DPERF * WINAPI D3DPERF_GetStatistics()
{
    if (!g_fWarnedNotInstrumented)
    {
        g_fWarnedNotInstrumented = TRUE;
        DbgPrint("D3D: D3DPERF counters read zero -- RXDK ships the retail D3D, "
                 "which carries no instrumentation. Push-buffer info is real; "
                 "event counters and the GPU profile are not gathered.\n");
    }

    return &g_D3DPerf;
}

void WINAPI D3DPERF_GetPushBufferInfo(D3DPUSHBUFFERINFO *pPushBufferInfo)
{
    if (pPushBufferInfo == NULL)
    {
        return;
    }

    memset(pPushBufferInfo, 0, sizeof(*pPushBufferInfo));

    if (g_pDevice == NULL)
    {
        return;
    }

    pPushBufferInfo->PushBufferSize  = CDevice::m_PushBufferSize;
    pPushBufferInfo->PushSegmentSize = CDevice::m_PushSegmentSize;
    pPushBufferInfo->PushSegmentCount =
        CDevice::m_PushSegmentSize
            ? CDevice::m_PushBufferSize / CDevice::m_PushSegmentSize
            : 0;

    pPushBufferInfo->pPushBase  = (DWORD *)g_pDevice->m_pPushBase;
    pPushBufferInfo->pPushLimit = (DWORD *)g_pDevice->m_pPushLimit;

    // "Guesstimate on count of bytes written since last D3DPERF_Reset", per the
    // header. Take it from how far the put pointer has advanced. The buffer is
    // circular, so a put pointer that has wrapped behind the reset mark has
    // covered the rest of the buffer plus the distance from the base.
    if (g_D3DPerf.m_pPutLocationAtReset != NULL)
    {
        DWORD *pPut   = (DWORD *)g_pDevice->m_Pusher.m_pPut;
        DWORD *pStart = g_D3DPerf.m_pPutLocationAtReset;

        if (pPut >= pStart)
        {
            pPushBufferInfo->PushBufferBytesWritten =
                (ULONGLONG)((BYTE *)pPut - (BYTE *)pStart);
        }
        else
        {
            pPushBufferInfo->PushBufferBytesWritten =
                (ULONGLONG)((BYTE *)g_pDevice->m_pPushLimit - (BYTE *)pStart) +
                (ULONGLONG)((BYTE *)pPut - (BYTE *)g_pDevice->m_pPushBase);
        }
    }
}

//
// GPU busy/idle profiling.
//
// The instrumented build arms a 100ns interrupt that samples which GPU units
// are busy, filling m_ProfileData. There is no such sampler here.
//
// Start returns BOOL, so the API has its own way of saying the profile did not
// start -- use it. Returning FALSE is better than pretending and handing back an
// all-zero profile that reads as "the GPU was idle for the whole run". The
// buffers are still cleared so a caller that ignores the result cannot see data
// left over from an earlier attempt.
//
BOOL WINAPI D3DPERF_StartPerfProfile()
{
    g_D3DPerf.m_TSCProfileStartTime = D3DPERF_ReadTSC();
    g_D3DPerf.m_ProfileSamples      = 0;

    memset(g_D3DPerf.m_ProfileData, 0, sizeof(g_D3DPerf.m_ProfileData));
    memset(g_D3DPerf.m_ProfileBusyCounts, 0, sizeof(g_D3DPerf.m_ProfileBusyCounts));

    return FALSE;
}

void WINAPI D3DPERF_StopPerfProfile()
{
    // Nothing to tear down -- nothing was ever started.
}

} // extern "C"
