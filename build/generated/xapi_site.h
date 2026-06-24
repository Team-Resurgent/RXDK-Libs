#ifndef RXDK_XAPI_SITE_H
#define RXDK_XAPI_SITE_H

/* Force-included for all libxapi translation units (Xbox-only). */

#ifndef RXDK_LIBXAPI_BUILD
#define RXDK_LIBXAPI_BUILD 1
#endif

#ifndef __C89_NAMELESS
#define __C89_NAMELESS
#endif

#ifndef _INTEGRAL_MAX_BITS
#define _INTEGRAL_MAX_BITS 64
#endif

#ifndef _XBOX
#define _XBOX 1
#endif

#ifndef _X86_
#define _X86_ 1
#endif

#ifndef i386
#define i386 1
#endif

#ifndef _M_IX86
#define _M_IX86 600
#endif

#ifndef _XAPI_
#define _XAPI_ 1
#endif

#define RXDK_CLANG_FIBER_TLS 1

#define RXDK_CLANG_TLS_IMAGE 1

#define NT_UP 1
#define XBOX 1

#ifndef NOD3D
#define NOD3D
#endif
#ifndef NODSOUND
#define NODSOUND
#endif

#include <stddef.h>
#ifndef wint_t
typedef unsigned short wint_t;
#endif
#include <stdarg.h>
#ifndef __gnuc_va_list
typedef __builtin_va_list __gnuc_va_list;
#endif

#ifndef NDEBUG
#define NDEBUG 1
#endif

#ifndef DBG
#define DBG 0
#endif

#endif
