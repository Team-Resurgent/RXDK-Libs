/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * XAPI methods exposed for use by drivers.
 */

#pragma once
#define _XAPIDRV_H_


#ifdef __cplusplus
extern "C" {
#endif

//
// Define API decoration for direct importing of DLL references.
//

#if !defined(_XAPI_)
#define XAPIDRVAPI DECLSPEC_IMPORT
#else
#define XAPIDRVAPI
#endif

//---------------------------------------------------------------------------------------------------------------
//  XAPI Driver APIS for reporting USB devices
//---------------------------------------------------------------------------------------------------------------
XAPIDRVAPI
VOID XdReportDeviceInsertionRemoval(
    PXPP_DEVICE_TYPE XppDeviceType,
    ULONG PortBit,
    BOOLEAN fInserted
    );

#define XDEVICE_ILLEGAL_PORT 32

#ifdef __cplusplus
}
#endif

