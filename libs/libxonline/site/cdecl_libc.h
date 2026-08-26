/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

//------------------------------------------------------------------------------
// cdecl_libc.h -- force-included FIRST into every libxonline translation unit.
//
// libxonline is built with -fdefault-calling-conv=stdcall (the Xbox Live client
// was /Gz, like the net stack it sits on) so its vendor kernel/net calls compile
// __stdcall and bind directly to libkernel.lib / libxnet.lib -- no cdecl->stdcall
// facades. With -ffreestanding, a plain libc prototype would inherit the stdcall
// default instead of the cdecl runtime convention, so the call site would emit
// e.g. _memcpy@12 (callee pops) while libc is compiled cdecl (_memcpy, caller
// pops) -> stack corruption.
//
// Declaring these __cdecl here, before picolibc's headers are seen, pins the
// convention. Note this alone is not enough, and was how the UIX samples wedged:
// when a TU goes on to include <stdlib.h> itself, clang silently takes the later
// defaulted declaration's convention (stdcall) over the explicit one here. The
// build therefore also passes -D__RXDK_LIBC_CDECL so picolibc's own prototypes
// carry __cdecl (see shared/picolibc/include/sys/cdefs.h), and
// scripts/Test-CallingConventions.ps1 fails a build if any such call survives.
//
// Mirrors libxnet/libd3d8/libdsound cdecl_libc.h. (__alldiv/__aullrem etc. are
// compiler 64-bit helpers with a fixed ABI -- not here.)
//------------------------------------------------------------------------------
#ifndef RXDK_XONLINE_CDECL_LIBC_H
#define RXDK_XONLINE_CDECL_LIBC_H

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cdecl)) void  *memcpy(void *, const void *, __SIZE_TYPE__);
__attribute__((cdecl)) void  *memmove(void *, const void *, __SIZE_TYPE__);
__attribute__((cdecl)) void  *memset(void *, int, __SIZE_TYPE__);
__attribute__((cdecl)) int    memcmp(const void *, const void *, __SIZE_TYPE__);
__attribute__((cdecl)) void  *memchr(const void *, int, __SIZE_TYPE__);
/* The heap. uix.cpp allocates its engine/plugin objects through these; left
   unpinned they compiled stdcall against a cdecl libc, so every call leaked 4
   bytes of ESP and the caller's epilogue returned through the wrong slot. */
__attribute__((cdecl)) void  *malloc(__SIZE_TYPE__);
__attribute__((cdecl)) void   free(void *);
__attribute__((cdecl)) void  *calloc(__SIZE_TYPE__, __SIZE_TYPE__);
__attribute__((cdecl)) void  *realloc(void *, __SIZE_TYPE__);
__attribute__((cdecl)) __SIZE_TYPE__ strlen(const char *);
__attribute__((cdecl)) char  *strcpy(char *, const char *);
__attribute__((cdecl)) char  *strncpy(char *, const char *, __SIZE_TYPE__);
__attribute__((cdecl)) char  *strcat(char *, const char *);
__attribute__((cdecl)) int    strcmp(const char *, const char *);
__attribute__((cdecl)) int    strncmp(const char *, const char *, __SIZE_TYPE__);
__attribute__((cdecl)) int    strncasecmp(const char *, const char *, __SIZE_TYPE__);
__attribute__((cdecl)) int    strcasecmp(const char *, const char *);
__attribute__((cdecl)) char  *strchr(const char *, int);
__attribute__((cdecl)) int    atoi(const char *);
__attribute__((cdecl)) long   atol(const char *);
__attribute__((cdecl)) long long atoll(const char *);
/* Wide-string helpers (xbosutil/msasn1). Pin cdecl so they bind to picolibc's
   _wcslen/_wcscpy rather than the stdcall-decorated _wcslen@4/_wcscpy@8. */
__attribute__((cdecl)) __SIZE_TYPE__ wcslen(const wchar_t *);
__attribute__((cdecl)) wchar_t *wcscpy(wchar_t *, const wchar_t *);

#ifdef __cplusplus
}
#endif

#endif // RXDK_XONLINE_CDECL_LIBC_H
