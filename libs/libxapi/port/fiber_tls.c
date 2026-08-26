/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Per-thread fiber TLS accessors for libxapi under Clang. The current-fiber and
 * thread-fiber-data slots live at fixed offsets within the thread's TLS image
 * (base = KeGetCurrentThread()->TlsData + sizeof(ULONG)); the offsets are pinned
 * in xapi_tls_layout.h to match Clang's .tls section order rather than taken at
 * runtime from emutls symbols.
 */

#include "bridge_k32.h"

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
