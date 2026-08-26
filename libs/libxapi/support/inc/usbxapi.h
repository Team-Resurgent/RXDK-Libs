/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * USB imports for xapi (Xbox).
 */

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
EXTERNUSB VOID USBD_Init(DWORD NumDeviceTypes, PXDEVICE_PREALLOC_TYPE DeviceTypes);

