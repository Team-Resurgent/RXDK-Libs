/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declarations for the fp_ floating-point functions that do not rely on the C
 * runtime. A translation unit that wants to avoid the CRT's floating-point
 * support must define "extern "C" int _fltused = 1;" in one .cpp file.
 */

// floating point operations
STDAPI_(long)   fp_ftol     (float flX);
STDAPI_(float)  fp_ltof     (long lX);
STDAPI_(float)  fp_fadd     (float fA, float fB);
STDAPI_(float)  fp_fsub     (float fA, float fB);
STDAPI_(float)  fp_fmul     (float fA, float fB);
STDAPI_(float)  fp_fdiv     (float fNum, float fDenom);
STDAPI_(float)  fp_fabs     (float flX);
STDAPI_(float)  fp_fsin     (float flX);
STDAPI_(float)  fp_fcos     (float flX);
STDAPI_(float)  fp_fpow     (float flX, float flY);
STDAPI_(float)  fp_flog2    (float flX);
STDAPI_(float)  fp_flog10   (float flX);
STDAPI_(float)  fp_fchs     (float flX);
STDAPI_(int)    fp_fcmp     (float flA, float flB);
STDAPI_(float)  fp_fmin     (float flA, float flB);
STDAPI_(float)  fp_fmax     (float flA, float flB);

