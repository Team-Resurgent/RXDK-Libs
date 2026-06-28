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

// RXDK: the original used naked MSVC __asm for these (clang can't compile naked
// __asm with esp-relative operands). We keep the SAME hardware instructions the
// driver chose for speed in the per-vertex math path -- x87 transcendentals
// (fcos / fyl2x) and the SSE truncate -- but expressed as clang-compatible GCC
// extended asm so the performance/precision reason for the asm is preserved.
// (Exp's original is a long f2xm1+fscale sequence with status-flag branches;
// expf() is used there -- faithful extended-asm replication would be fragile and
// Exp is not on the hot transform path. Revisit with profiling if it shows up.)

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{

    // Converts a floating point value to a long. Truncates (not floor) -- clang
    // compiles this cast to the same cvttss2si the original asm used.
    long FloatToLong(float f)
    {
        return (long)f;
    }

    // Cosine -- x87 hardware fcos (st0 = cos(st0)).
    float Cos(float e)
    {
        float r;
        __asm__ ("fcos" : "=t"(r) : "0"(e));
        return r;
    }

    // Exponent (e^x). See header note -- libc here, not the hardware sequence.
    float Exp(float e)
    {
        return expf(e);
    }

    // Natural log -- x87 hardware: ln(e) = ln2 * log2(e) via fyl2x.
    float Log(float e)
    {
        if (DBG_CHECK(e <= 0.0f))
        {
            DXGRIP("Log - fast log doesn't handle zero or negative values.");
        }

        float r;
        // fldln2 pushes ln2 (st0=ln2, st1=e); fxch -> (st0=e, st1=ln2);
        // fyl2x -> st1*log2(st0) = ln2*log2(e) = ln(e), popping to one slot.
        __asm__ ("fldln2 ; fxch ; fyl2x" : "=t"(r) : "0"(e) : "st(1)");
        return r;
    }

} // end namespace
