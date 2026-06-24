#ifndef RXDK_XAPI_TITLE_SITE_H
#define RXDK_XAPI_TITLE_SITE_H

/*
 * Force-included for title samples (xapi-smoke, etc.).
 * Leak xAPI order: xboxkrnl first, NT_INCLUDED, then xtl via include/xtl.h.
 */

#ifndef _XBOX
#define _XBOX 1
#endif
#ifndef _X86_
#define _X86_ 1
#endif
#ifndef _M_IX86
#define _M_IX86 600
#endif
#ifndef _WIN32
#define _WIN32 1
#endif

#include <xboxkrnl/xboxkrnl.h>

#ifndef _NTURTL_
#define _NTURTL_
#endif

#include <xapi/heap_rtl.h>

#ifndef NT_INCLUDED
#define NT_INCLUDED
#endif

#define RXDK_CLANG_FIBER_TLS 1

#endif /* RXDK_XAPI_TITLE_SITE_H */
