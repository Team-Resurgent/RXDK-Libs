/* RXDK libxapi link compat: TEB, string aliases, debug stubs. */

#include <xboxkrnl/xboxkrnl.h>
#include <string.h>

#ifndef FASTCALL
#define FASTCALL __attribute__((fastcall))
#endif

#ifndef WINAPI
#define WINAPI __stdcall
#endif

typedef struct _TEB *PTEB;

extern char *WINAPI lstrcpynA(char *dst, const char *src, int cap);

PTEB NTAPI NtCurrentTeb(void)
{
    PTEB teb;
    __asm__ volatile("movl %%fs:0x18, %0" : "=r"(teb));
    return teb;
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

void xbox_runtime_init(void);

void __cdecl xdk_xbox_crt_early_init(void)
{
}

void __cdecl xdk_xbox_crt_startup(void)
{
    extern void __main(void);
    __main();
}

/* Link-time fallbacks when macros are not visible across TUs. */
#undef ocslen
#undef ocscpy
#undef lstrcpynO
#undef RtlInitObjectString

size_t ocslen(const char *s)
{
    return strlen(s);
}

char *ocscpy(char *d, const char *s)
{
    return strcpy(d, s);
}

char *lstrcpynO(char *dst, const char *src, int cap)
{
    return lstrcpynA(dst, src, cap);
}

VOID NTAPI RtlInitObjectString(POBJECT_STRING dst, PCSTR src)
{
    RtlInitAnsiString((PANSI_STRING)dst, src);
}

int __cdecl _snwprintf(wchar_t *buf, size_t n, const wchar_t *fmt, ...)
{
    (void)buf;
    (void)n;
    (void)fmt;
    return 0;
}

int __cdecl _snprintf(char *buf, size_t n, const char *fmt, ...)
{
    (void)buf;
    (void)n;
    (void)fmt;
    return 0;
}
