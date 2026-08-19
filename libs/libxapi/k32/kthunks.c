#include "bridge_k32.h"
#include "basedll.h"
#include <stdio.h>
#include <wchar.h>
#include "ms_printf.h"


VOID
__attribute__((__stdcall__))
DebugBreak()
{
    DbgBreakPoint();
}

VOID
__attribute__((__stdcall__))
GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
    KeQuerySystemTime((PLARGE_INTEGER) lpSystemTimeAsFileTime);
}

int
WINAPIV
wsprintfW(LPWSTR lpOut, LPCWSTR lpFmt, ...)
{
    va_list arglist;
    int ret;

    va_start(arglist, lpFmt);
    ret = wvsprintfW(lpOut, lpFmt, arglist);

    va_end(arglist);
    return ret;
}

int
WINAPIV
wsprintfA(LPSTR lpOut, LPCSTR lpFmt, ...)
{
    va_list arglist;
    int ret;

    va_start(arglist, lpFmt);
    ret = wvsprintfA(lpOut, lpFmt, arglist);

    va_end(arglist);
    return ret;
}

//
// We need a wrapper for wvsprintf() (as opposed to forwarding it to NTOSKRNL)
// because the calling convention is not the same as vwsprintf()
//

//
// wvsprintf takes no buffer size: Win32 documents 1024 characters as the most
// it will ever produce, so that is the bound handed to the C99 formatter.
//
#define WVSPRINTF_MAX_OUTPUT 1024

int
__attribute__((__stdcall__))
wvsprintfW(
    OUT LPWSTR lpOut,
    IN LPCWSTR lpFmt,
    IN va_list arglist)
{
    wchar_t stack[RXDK_MS_FORMAT_STACK];
    wchar_t *heap;
    const wchar_t *use;
    int ret;

    use = __rxdk_ms_wformat(lpFmt, stack, RXDK_MS_FORMAT_STACK, &heap);
    ret = vswprintf(lpOut, WVSPRINTF_MAX_OUTPUT, use, arglist);
    __rxdk_ms_format_free(heap);

    return ret;
}

int
__attribute__((__stdcall__))
wvsprintfA(
    OUT LPSTR lpOut,
    IN LPCSTR lpFmt,
    IN va_list arglist)
{
    char stack[RXDK_MS_FORMAT_STACK];
    char *heap;
    const char *use;
    int ret;

    use = __rxdk_ms_format(lpFmt, stack, RXDK_MS_FORMAT_STACK, &heap);
    ret = vsnprintf(lpOut, WVSPRINTF_MAX_OUTPUT, use, arglist);
    __rxdk_ms_format_free(heap);

    return ret;
}

ULONG
WINAPIV
DebugPrint(PCHAR Format, ...)
{
    va_list arglist;
    CHAR string[MAX_PATH];
    ULONG ret;

    va_start(arglist, Format);
    ret = _vsnprintf(string, sizeof(string), Format, arglist);
    OutputDebugStringA(string);

    va_end(arglist);
    return ret;
}
