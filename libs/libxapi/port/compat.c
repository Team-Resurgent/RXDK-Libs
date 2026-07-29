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

int __cdecl _stricmp(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = (char)((*a >= 'A' && *a <= 'Z') ? *a + 32 : *a);
        char cb = (char)((*b >= 'A' && *b <= 'Z') ? *b + 32 : *b);
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int __cdecl _wcsicmp(const wchar_t *a, const wchar_t *b)
{
    while (*a && *b) {
        wchar_t ca = (*a >= L'A' && *a <= L'Z') ? *a + 32 : *a;
        wchar_t cb = (*b >= L'A' && *b <= L'Z') ? *b + 32 : *b;
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        a++;
        b++;
    }
    return (int)*a - (int)*b;
}

int __cdecl _snwprintf(wchar_t *buf, size_t n, const wchar_t *fmt, ...)
{
    va_list ap;
    int r;

    if (!buf || n == 0) {
        return 0;
    }
    va_start(ap, fmt);
    r = vswprintf(buf, n, fmt, ap);
    va_end(ap);
    if (r < 0) {
        buf[0] = L'\0';
        return -1;
    }
    if ((size_t)r >= n) {
        buf[n - 1] = L'\0';
    }
    return r;
}

/*
 * MSVC's narrow printf-to-buffer family.
 *
 * The size argument does NOT mean what it means in C99, and getting this wrong
 * is very hard to trace back here:
 *
 *   C99  vsnprintf(buf, size, ..) writes at most size-1 characters plus a NUL
 *                                 and returns the length it would have needed.
 *   MSVC _vsnprintf(buf, count, ..) writes at most count characters, terminates
 *                                 only when the result fits in fewer than count,
 *                                 and returns -1 when it had to truncate.
 *
 * Xbox-era code is written to the MSVC rule, so the common idiom
 *
 *     len = _vsnprintf(NULL, 0, fmt, ap);
 *     buf = malloc(len + 1);
 *     _vsnprintf(buf, len, fmt, ap);   // len chars, no NUL
 *     buf[len] = 0;
 *
 * loses its last character if the size argument is treated as C99's. (That
 * surfaced as a title rendering wrong glyphs, because its lookup keys were
 * built this way and "65" became "6".)
 *
 * One deliberate extension: a NULL buffer or zero count returns the required
 * length instead of MSVC's -1, so the measuring idiom above works. Nothing can
 * usefully depend on the -1, and code ported from C99 measures this way.
 * _vscprintf/_scprintf are the portable spelling of the same question.
 */

int __cdecl _vscprintf(const char *fmt, va_list ap)
{
    return vsnprintf(NULL, 0, fmt, ap);
}

int __cdecl _scprintf(const char *fmt, ...)
{
    va_list ap;
    int r;

    va_start(ap, fmt);
    r = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    return r;
}

int __cdecl _vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
    va_list measure;
    int needed;

    if (!buf || n == 0) {
        return vsnprintf(NULL, 0, fmt, ap);
    }

    va_copy(measure, ap);
    needed = vsnprintf(NULL, 0, fmt, measure);
    va_end(measure);

    if (needed < 0) {
        buf[0] = '\0';
        return -1;
    }

    if ((size_t)needed < n) {
        /* Fits with room for the terminator: C99 behaviour is what MSVC does. */
        return vsnprintf(buf, n, fmt, ap);
    }

    /*
     * Truncating. MSVC fills all n bytes and writes no terminator, so we cannot
     * let vsnprintf stop a character short -- format the whole string and copy
     * the leading n bytes over.
     */
    {
        char *scratch = (char *)malloc((size_t)needed + 1);

        if (scratch) {
            vsnprintf(scratch, (size_t)needed + 1, fmt, ap);
            memcpy(buf, scratch, n);
            free(scratch);
        } else {
            /* Out of memory: a short-by-one result beats leaving buf untouched. */
            vsnprintf(buf, n, fmt, ap);
        }
    }

    return -1;
}

int __cdecl _snprintf(char *buf, size_t n, const char *fmt, ...)
{
    va_list ap;
    int r;

    va_start(ap, fmt);
    r = _vsnprintf(buf, n, fmt, ap);
    va_end(ap);

    return r;
}
