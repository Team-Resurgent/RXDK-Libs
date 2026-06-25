/* RXDK libxapi link compat: TEB, string aliases, debug stubs. */

#include <xboxkrnl/xboxkrnl.h>
#include <stdarg.h>
#include <stdio.h>

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

int __cdecl _snprintf(char *buf, size_t n, const char *fmt, ...)
{
    va_list ap;
    int r;

    if (!buf || n == 0) {
        return 0;
    }
    va_start(ap, fmt);
    r = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    if (r < 0) {
        buf[0] = '\0';
        return -1;
    }
    if ((size_t)r >= n) {
        buf[n - 1] = '\0';
    }
    return r;
}
