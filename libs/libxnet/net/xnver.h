/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Stamps the source file that includes it with the library's version structure.
 * Deliberately single-include: re-inclusion is a build error.
 */

#ifdef  __XNVER_H__
#error "xnver.h should not be included more than once"
#endif
#define __XNVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <xboxverp.h>

#ifdef XNET_FEATURE_XBOX
#pragma data_seg(push)
#pragma data_seg(".XBLD$V")
#endif

#if defined(XNET_BUILD_LIBX) && DBG==0
    #define VERXNET()   VERGEN(XNetBuildNumber,     'X','N','E','T',0,0,0,0)
#elif defined(XNET_BUILD_LIBX) 
    #define VERXNET()   VERGEN(XNetBuildNumberD,    'X','N','E','T','D',0,0,0)
#elif defined(XNET_BUILD_LIBXS) && DBG==0
    #define VERXNET()   VERGEN(XNetBuildNumberS,    'X','N','E','T','S',0,0,0)
#elif defined(XNET_BUILD_LIBXS)
    #define VERXNET()   VERGEN(XNetBuildNumberSD,   'X','N','E','T','S','D',0,0)
#elif defined(XNET_BUILD_LIBXW) && DBG==0
    #define VERXNET()   VERGEN(XNetBuildNumberW,    'X','N','E','T','S',0,0,0)
#elif defined(XNET_BUILD_LIBXW)
    #define VERXNET()   VERGEN(XNetBuildNumberWD,   'X','N','E','T','S','D',0,0)
#elif defined(XNET_BUILD_LIBM) && DBG==0
    #define VERXNET()   VERGEN(XNetBuildNumberM,    'X','N','E','T','M',0,0,0)
#elif defined(XNET_BUILD_LIBM)
    #define VERXNET()   VERGEN(XNetBuildNumberMD,   'X','N','E','T','M','D',0,0)
// RXDK: the leak stamps the LIBO build with XOnlineBuildNumber, which COLLIDES
// with the identically-named stamp libxonline emits -- a title linking both gets
// two definitions of one symbol, resolved to whichever archive member the linker
// happens to pull. Retail 5849 does not do this: every xnet*.lib there stamps
// XNetBuildNumber* and only xonline*.lib stamps XOnlineBuildNumber*, verified by
// reading the shipped libs. So the online xnet build gets its own xnet-flavoured
// name, leaving XOnlineBuildNumber to libxonline alone.
#elif defined(XNET_BUILD_LIBO) && DBG==0
    #define VERXNET()   VERGEN(XNetBuildNumberO,    'X','N','E','T','O',0,0,0)
#elif defined(XNET_BUILD_LIBO)
    #define VERXNET()   VERGEN(XNetBuildNumberOD,   'X','N','E','T','O','D',0,0)
#elif defined(XNET_BUILD_LIBOS) && DBG==0
    #define VERXNET()   VERGEN(XOnlineBuildNumberS, 'X','O','N','L','I','N','E','S')
#elif defined(XNET_BUILD_LIBOS)
    #define VERXNET()   VERGEN(XOnlineBuildNumberSD,'X','O','N','L','I','N','S','D')
#elif defined(XNET_BUILD_LIBOW) && DBG==0
    #define VERXNET()   VERGEN(XOnlineBuildNumberW, 'X','O','N','L','I','N','E','W')
#elif defined(XNET_BUILD_LIBOW)
    #define VERXNET()   VERGEN(XOnlineBuildNumberWD,'X','O','N','L','I','N','W','D')
#else
    #error "Don't know how to make VERXNET"
#endif

#if DBG
#define VERDBG  0x8000
#else
#define VERDBG  0x0000
#endif

//
// XNET_BUILD_LIBOS is not conditionally approved yet - when it is, uncomment the
// line below
//

// #if DBG==0 && (defined(XNET_BUILD_LIBXS) || defined(XNET_BUILD_LIBOS))
#if DBG==0 && (defined(XNET_BUILD_LIBXS))
#define VERSEC  0x2000
#else
#define VERSEC  0x0000
#endif

#define VERGEN(var,a1,a2,a3,a4,a5,a6,a7,a8) comment(linker, "/include:_" # var)
#pragma VERXNET()
#undef  VERGEN
#define VERGEN(var,a1,a2,a3,a4,a5,a6,a7,a8) var[8] = { (a1) | ((a2) << 8), (a3) | ((a4) << 8), (a5) | ((a6) << 8), (a7) | ((a8) << 8), VER_PRODUCTVERSION }
__declspec(selectany) unsigned short VERXNET();
#undef  VERGEN
#undef  VERDBG
#undef  VERSEC
#undef  VERXNET

#ifdef XNET_FEATURE_XBOX
#pragma data_seg(pop)
#endif

#ifdef __cplusplus
};
#endif
