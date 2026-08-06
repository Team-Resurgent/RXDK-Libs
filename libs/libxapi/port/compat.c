#include "bridge_k32.h"
/* RXDK libxapi link compat: TEB, string aliases, debug stubs. */

#include <xboxkrnl/xboxkrnl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> /* malloc/free: _vsnprintf's truncation path */
#include <string.h> /* memcpy */

typedef struct _TEB *PTEB;

PTEB __stdcall NtCurrentTeb(void)
{
    PTEB teb;
    __asm__ volatile("movl %%fs:0x18, %0" : "=r"(teb));
    return teb;
}

void xapi_runtime_init(void)
{
}

void __cdecl xdk_xbox_crt_early_init(void)
{
}

void __cdecl xdk_xbox_crt_startup(void)
{
}

#undef ocslen
#undef ocscpy

size_t ocslen(const char *s)
{
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

char *ocscpy(char *d, const char *s)
{
    char *r = d;
    while ((*d++ = *s++) != '\0') {
    }
    return r;
}

/*
 * The MSVC-name CRT functions (_stricmp/_wcsicmp/_snprintf/_snwprintf/
 * _scprintf/_vscprintf/_vsnprintf) used to live here. They are CRT, and 5849
 * ships them in libc, so they moved to libs/libc/xbox/msvc_crt.c.
 */
