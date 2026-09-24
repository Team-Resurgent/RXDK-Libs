/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
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

