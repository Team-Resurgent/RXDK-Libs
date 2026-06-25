#include "bridge_k32.h"
/*
 * RXDK: per-thread fiber TLS for Clang/libxapi.
 *
 * Templates live in xapi_fiber_tls_data.c. Offsets are fixed in xapi_tls_layout.h
 * (Clang .tls$$ section order); do not use runtime & on emutls symbols.
 */

#include "basedll.h"
#pragma hdrstop

int rxdk_xapi_current_fiber_tls_disp = RXDK_TLS_IMAGE_OFF_CURRENT_FIBER;

static PBYTE rxdk_tls_image_base(void)
{
    return (PBYTE)KeGetCurrentThread()->TlsData + sizeof(ULONG);
}

PVOID *rxdk_xapi_current_fiber_slot(void)
{
    return (PVOID *)(rxdk_tls_image_base() + RXDK_TLS_IMAGE_OFF_CURRENT_FIBER);
}

XFIBER *rxdk_xapi_thread_fiber_data_slot(void)
{
    return (XFIBER *)(rxdk_tls_image_base() + RXDK_TLS_IMAGE_OFF_THREAD_FIBER_DATA);
}
