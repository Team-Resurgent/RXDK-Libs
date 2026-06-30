//------------------------------------------------------------------------------
// cdecl_libc.h -- force-included FIRST into every libdsound translation unit.
//
// libdsound is built with -fdefault-calling-conv=stdcall (so its vendor kernel
// calls compile __stdcall and bind directly to libkernel.lib -- no cdecl->stdcall
// facades) AND -ffreestanding. -ffreestanding disables clang's builtin libc
// recognition, so a plain libc prototype inherits the stdcall default instead of
// the cdecl runtime convention: the call site emits e.g. _memcpy@12 (callee pops)
// while libc/libm are compiled cdecl (_memcpy, caller pops). lld fuzzy-resolves
// the name but the two sides disagree on stack cleanup -> corruption.
//
// Declaring these __cdecl here, before picolibc's <string.h>/<math.h> are seen,
// pins the correct convention (first declaration's CC wins). Mirrors libd3d8's
// cdecl_libc.h. The set is exactly the libc/libm symbols libdsound leaves
// undefined (mem*, abs, the double/float transcendentals); extend if a future
// dsound path introduces more.
//------------------------------------------------------------------------------
#ifndef RXDK_DSOUND_CDECL_LIBC_H
#define RXDK_DSOUND_CDECL_LIBC_H

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cdecl)) void *memcpy(void *, const void *, __SIZE_TYPE__);
__attribute__((cdecl)) void *memset(void *, int, __SIZE_TYPE__);
__attribute__((cdecl)) int   memcmp(const void *, const void *, __SIZE_TYPE__);
__attribute__((cdecl)) int    abs(int);
__attribute__((cdecl)) double acos(double);
__attribute__((cdecl)) double cos(double);
__attribute__((cdecl)) double sqrt(double);
__attribute__((cdecl)) double log10(double);
__attribute__((cdecl)) float  fabsf(float);
__attribute__((cdecl)) float  log10f(float);

#ifdef __cplusplus
}
#endif

#endif // RXDK_DSOUND_CDECL_LIBC_H
