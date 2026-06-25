/*
 * RXDK: Clang emits __declspec(thread) via fs:[0x2C]; Xbox xAPI uses a different
 * TLS layout. Route GetLastError/SetLastError through the copied TLS template
 * in KeGetCurrentThread()->TlsData instead of touching TEB fs:[0x2C].
 */

#include "basedll.h"
#pragma hdrstop

static DWORD *rxdk_last_error_ptr(void)
{
    PBYTE tls_data;

    tls_data = (PBYTE)KeGetCurrentThread()->TlsData + sizeof(ULONG);
    return (DWORD *)(tls_data + RXDK_TLS_IMAGE_OFF_LAST_ERROR);
}

BOOL XapiIsXapiThread(void);

DWORD __stdcall GetLastError(void)
{
    if (!XapiIsXapiThread()) {
        XDBGERR("XAPI", "GetLastError() called on non-XAPI thread");
    }
    return *rxdk_last_error_ptr();
}

VOID __stdcall SetLastError(DWORD dwErrCode)
{
    if (!XapiIsXapiThread()) {
        XDBGERR("XAPI", "SetLastError() called on non-XAPI thread");
    }
    *rxdk_last_error_ptr() = (ULONG)dwErrCode;
}
