/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// Instantiates the ActiveX-scripting-related GUIDs the dmscript module needs
// (via INITGUID), plus a local shim so urlmon.h/activscp.h compile here.
#define INITGUID
#include <debug.h>
#include <guiddef.h>

// Stupic HACK to get urlmon.h to compile as part of the activscript headers.
typedef struct _SECURITY_ATTRIBUTES
    {
    DWORD nLength;
    DWORD lpSecurityDescriptor;
    BOOL bInheritHandle;
    }	SECURITY_ATTRIBUTES;
#include <activscp.h>
//#endif

// RXDK: GUID_NULL is NOT defined here -- libxapi's uuid component (uuid/cguid.c) owns it,
// and defining it again makes every link pulling both libraries fail on a duplicate symbol.
// Same reason the standard OLE IIDs were dropped from dmguids/comguids_rxdk.cpp.
