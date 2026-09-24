/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Buffer base class: logic shared between the Index, Vertex, and Command
 * buffer types.
 */

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{

//----------------------------------------------------------------------------
// Helper to create an instance of a buffer.
//
HRESULT CreateVertexIndexOrPushBuffer(
    DWORD Type,
    DWORD Size, 
    void **ppBuffer);


} // end namespace
