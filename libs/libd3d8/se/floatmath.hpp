/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Private implementation of floating-point math routines used by the driver.
 */

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{
    // Converts a floating point value to a long.
    long FloatToLong(float f);

    // Cosine.
    float Cos(float e);

    // Exponent
    float Exp(float e);

    // Log
    float Log(float e);

} // end namespace
