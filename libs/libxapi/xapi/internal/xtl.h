#ifndef RXDK_INCLUDE_XTL_H
#define RXDK_INCLUDE_XTL_H

#if defined(XAPI_UUID_BUILD)
#include <sdk/xtl.h>
#else

/*
 * Minimal title-facing includes for libxapi translation units (replaces leak
 * xtl.h during the xAPI port).
 */
#ifndef _XTL_
#define _XTL_
#endif
#ifndef _NTURTL_
#define _NTURTL_
#endif
#ifndef _INC_XTL
#define _INC_XTL
#endif
#ifndef _INC_WINDOWS
#define _INC_WINDOWS
#endif
#ifndef WINVER
#define WINVER 0x0500
#endif

#include <stdarg.h>

#ifndef _NTDEF_
#include "win32_bridge.h"
#else
#include "winnt_skipped.h"
#endif
#include "heap_rtl.h"
#include <windef.h>
#include <winbase.h>
#include <xbox.h>
#include <xkbd.h>

#endif

#endif
