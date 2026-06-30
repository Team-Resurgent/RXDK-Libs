/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    xapidrv.h

Abstract:

    XAPI methods exposed for use of drivers.

--*/

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
// STDCALL: defined cdecl-default in the k32 core (xpp.c) but called from the
// stdcall-default USB driver (usbd/tree.cpp); pin so both agree (else _Xd...@12
// vs _Xd... fuzzy-resolves with a stack imbalance).
XAPIDRVAPI
VOID STDCALL XdReportDeviceInsertionRemoval(
    PXPP_DEVICE_TYPE XppDeviceType,
    ULONG PortBit,
    BOOLEAN fInserted
    );

#define XDEVICE_ILLEGAL_PORT 32

#ifdef __cplusplus
}
#endif

