/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Instantiates the D3DX8 interface GUIDs (IID_ID3DXSprite, IID_ID3DXEffect,
 * IID_ID3DXBaseMesh and friends). The headers declare them with DEFINE_GUID,
 * which only emits the actual data in the translation unit that sets INITGUID
 * before the declaration is seen. The D3DX8 headers do not compile as C, so
 * core/init.c (which handles the .X-file GUIDs) cannot include them; this C++
 * translation unit defines INITGUID and pulls in d3dx8.h to provide the GUID
 * data any title referencing these interfaces needs at link time.
 */

#define INITGUID

#include <xtl.h>
#include <d3dx8.h>
