/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Enumerator object class for adapter/mode enumeration.
 */

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{

class CEnum : public Direct3D
{
public:

    D3DFORMAT MapUnknownFormat(
        UINT         iAdapter,
        DWORD        Usage,
        D3DFORMAT    Format,
        D3DDEVTYPE   Type,
        D3DFORMAT    DisplayFormat) const;
}; 


} // end namespace
