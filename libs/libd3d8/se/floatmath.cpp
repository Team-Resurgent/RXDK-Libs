/*==========================================================================
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       floatmath.cpp
 *  Content:    Private impementation of floating-point math routines.
 *
 ***************************************************************************/

#include "precomp.hpp"
#include <math.h>

// RXDK: the original implemented these four routines as naked x87/SSE asm
// (cvttss2si / fcos / f2xm1+fscale / fyl2x). We have a full libc math (picolibc),
// so use it instead of porting the assembly -- behaviourally identical for the
// driver's use (single-precision cos/exp/log + truncating float->long).

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{

    // Converts a floating point value to a long. Truncates (not floor), matching
    // the original cvttss2si.
    long FloatToLong(float f)
    {
        return (long)f;
    }

    // Cosine.
    float Cos(float e)
    {
        return cosf(e);
    }

    // Exponent (e^x).
    float Exp(float e)
    {
        return expf(e);
    }

    // Natural log.
    float Log(float e)
    {
        if (DBG_CHECK(e <= 0.0f))
        {
            DXGRIP("Log - fast log doesn't handle zero or negative values.");
        }

        return logf(e);
    }

} // end namespace
