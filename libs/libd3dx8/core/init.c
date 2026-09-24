/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Instantiates the .X-file (DirectX Retained Mode) GUIDs and template data by
 * defining INITGUID and pulling in rmxfguid.h and rmxftmpl.h. The d3dx8 headers
 * do not compile as C because they do not define INTERFACE for the interfaces
 * they declare; since this translation unit only needs the GUID and template
 * data, INTERFACE is aliased to IUnknown so the headers parse under C. The D3DX8
 * interface IIDs themselves are instantiated separately in d3dxguids.cpp.
 */

#define INITGUID

#define INTERFACE IUnknown

#include <xtl.h>
#include <rmxfguid.h>
#include <rmxftmpl.h>

