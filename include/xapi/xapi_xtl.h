#ifndef RXDK_XAPI_XTL_H
#define RXDK_XAPI_XTL_H

/*
 * Minimal title-facing includes for libxapi translation units.
 *
 * Replaces leak xtl.h during the xAPI port. Staged include/xdk/ headers are
 * transitional — migrate declarations here (or into xapi.h / generated stubs)
 * until include/xdk can be removed.
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
#include <xapi/xdk_bridge.h>
#else
#include <xapi/winnt_skipped.h>
#endif
#include <xapi/heap_rtl.h>
#include <windef.h>
#include <winbase.h>
#include <xbox.h>

#endif /* RXDK_XAPI_XTL_H */
