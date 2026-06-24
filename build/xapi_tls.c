/*
 * RXDK replacement for vendor k32/tls.c.
 *
 * Clang maps __declspec(thread) to fs:[0x2C]; Xbox xAPI stores per-thread data
 * in KeGetCurrentThread()->TlsData. Slot storage lives in xapi_tls_data.c;
 * offsets are fixed in xapi_tls_layout.h.
 */

#include "basedll.h"
#pragma hdrstop

ULONG XapiTlsAllocBitmap[TLS_MINIMUM_AVAILABLE / 32] = {
    0xFFFFFFFF,
#if TLS_MINIMUM_AVAILABLE > 32
    0xFFFFFFFF,
#endif
};

static ULONG rxdk_tls_slot_byte_offset(DWORD dwTlsIndex)
{
    return (ULONG)(sizeof(ULONG) + (ULONG)RXDK_TLS_IMAGE_OFF_TLS_SLOTS +
                   (ULONG)dwTlsIndex * (ULONG)sizeof(PVOID));
}

static PVOID *rxdk_tls_slots_ptr(void)
{
    PBYTE tls_data;

    tls_data = (PBYTE)KeGetCurrentThread()->TlsData + sizeof(ULONG);
    return (PVOID *)(tls_data + RXDK_TLS_IMAGE_OFF_TLS_SLOTS);
}

DWORD APIENTRY TlsAlloc(VOID)
{
    DWORD dwBitmapIndex;
    DWORD dwBitIndex;
    DWORD dwTlsIndex;

    XapiAcquireProcessLock();

    dwTlsIndex = 0xffffffff;

    for (dwBitmapIndex = 0; dwBitmapIndex < ARRAYSIZE(XapiTlsAllocBitmap); dwBitmapIndex++) {
        if (XapiTlsAllocBitmap[dwBitmapIndex] != 0) {
            dwBitIndex = RtlFindFirstSetRightMember(XapiTlsAllocBitmap[dwBitmapIndex]);
            XapiTlsAllocBitmap[dwBitmapIndex] &= ~(1 << dwBitIndex);
            dwTlsIndex = (dwBitmapIndex * 32) + dwBitIndex;
            break;
        }
    }

    XapiReleaseProcessLock();

    return dwTlsIndex;
}

LPVOID APIENTRY TlsGetValue(DWORD dwTlsIndex)
{
    RIP_ON_NOT_TRUE("TlsGetValue()", (dwTlsIndex < TLS_MINIMUM_AVAILABLE));

    return rxdk_tls_slots_ptr()[dwTlsIndex];
}

BOOL APIENTRY TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
    RIP_ON_NOT_TRUE("TlsSetValue()", (dwTlsIndex < TLS_MINIMUM_AVAILABLE));

    rxdk_tls_slots_ptr()[dwTlsIndex] = lpTlsValue;

    return TRUE;
}

BOOL APIENTRY TlsFree(DWORD dwTlsIndex)
{
    DWORD dwBitmapIndex;
    DWORD dwBitIndex;
    ULONG TlsDataRelativeOffset;

    RIP_ON_NOT_TRUE("TlsFree()", (dwTlsIndex < TLS_MINIMUM_AVAILABLE));

    XapiAcquireProcessLock();

    dwBitmapIndex = dwTlsIndex / 32;
    dwBitIndex = dwTlsIndex % 32;

    XapiTlsAllocBitmap[dwBitmapIndex] |= (1 << dwBitIndex);

    TlsDataRelativeOffset = rxdk_tls_slot_byte_offset(dwTlsIndex);
    *((PVOID *)((ULONG_PTR)KeGetCurrentThread()->TlsData + TlsDataRelativeOffset)) = NULL;

    XapiReleaseProcessLock();

    return TRUE;
}
