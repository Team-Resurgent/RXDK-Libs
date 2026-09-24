/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Base class for all texture objects; texture management is handled at this
 * level.
 */

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{
 
//----------------------------------------------------------------------------
// Helper to create an instance of any texture.
//
HRESULT CreateTexture(
    DWORD Width,
    DWORD Height,
    DWORD Depth,
    DWORD Levels,
    DWORD Usage,
    D3DFORMAT Format,
    bool isCubeMap,
    bool isVolumeTexture,
    D3DBaseTexture **ppTexture
    );

} // end namespace
