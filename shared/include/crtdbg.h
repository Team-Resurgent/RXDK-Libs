/*
 * RXDK minimal <crtdbg.h> shim.
 *
 * Shadows zig's MinGW crtdbg.h (which pulls _mingw.h -> vadefs.h and fails under
 * our freestanding flags). The MSVC CRT debug header is only used by the leak
 * sources for debug assertions/heap tracking, all of which are no-ops in our
 * retail (NDEBUG) build (e.g. xgraphics/dxtc/s3_intrf.cpp includes it
 * unconditionally but only uses _ASSERTE under #if _DEBUG). Provide the common
 * macros as no-ops so the include is self-sufficient.
 */
#ifndef RXDK_CRTDBG_H
#define RXDK_CRTDBG_H

#ifndef _ASSERT
#define _ASSERT(expr)            ((void)0)
#endif
#ifndef _ASSERTE
#define _ASSERTE(expr)           ((void)0)
#endif
#ifndef _ASSERT_EXPR
#define _ASSERT_EXPR(expr, msg)  ((void)0)
#endif
#ifndef _RPT0
#define _RPT0(t, msg)            ((void)0)
#define _RPT1(t, msg, a)         ((void)0)
#define _RPT2(t, msg, a, b)      ((void)0)
#define _RPT3(t, msg, a, b, c)   ((void)0)
#define _RPT4(t, msg, a, b, c, d) ((void)0)
#endif
#ifndef _RPTF0
#define _RPTF0(t, msg)           ((void)0)
#define _RPTF1(t, msg, a)        ((void)0)
#define _RPTF2(t, msg, a, b)     ((void)0)
#define _RPTF3(t, msg, a, b, c)  ((void)0)
#define _RPTF4(t, msg, a, b, c, d) ((void)0)
#endif

/* Debug heap shims -> plain CRT (no tracking). */
#ifndef _malloc_dbg
#define _malloc_dbg(size, type, file, line)        malloc(size)
#define _calloc_dbg(n, size, type, file, line)     calloc(n, size)
#define _realloc_dbg(p, size, type, file, line)    realloc(p, size)
#define _free_dbg(p, type)                         free(p)
#endif
#ifndef _CrtCheckMemory
#define _CrtCheckMemory()        (1)
#endif

#endif /* RXDK_CRTDBG_H */
