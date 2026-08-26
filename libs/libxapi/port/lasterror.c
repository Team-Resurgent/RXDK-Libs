/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * GetLastError / SetLastError for libxapi. Clang emits thread-locals via the
 * TEB (fs:[0x2C]), but the Xbox xAPI uses a different TLS layout, so the
 * last-error value is read from and written to a fixed offset in the thread's
 * copied TLS image (KeGetCurrentThread()->TlsData) instead of the TEB.
 */

#include "bridge_k32.h"

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
