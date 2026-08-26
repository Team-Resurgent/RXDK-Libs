/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
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
