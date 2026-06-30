/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    usbxapi.h

    Generated from usb.x

Abstract:

    USB imports especially for xapi

Environment:

    Xbox

--*/

#pragma once
#define __USB_X__



#define EXTERNUSB extern

//
//	USB drivers, and XAPI code modules that rely on USB all go into 
//	the XPP section.
//
#pragma code_seg(".XPPCODE")
#pragma data_seg(".XPP$Data")
#pragma const_seg(".XPPRDATA")


//------------------------------------------------
//  Entry Point XAPI must call
//------------------------------------------------
EXTERNUSB VOID STDCALL USBD_Init(DWORD NumDeviceTypes, PXDEVICE_PREALLOC_TYPE DeviceTypes);

