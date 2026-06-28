//------------------------------------------------------------------------------
// cdecl_libc.h -- force-included FIRST into every libd3d8 translation unit.
//
// libd3d8 is built with -fdefault-calling-conv=stdcall (the Xbox D3D8 was built
// /Gz) AND -ffreestanding. -ffreestanding disables clang's builtin recognition
// of the C library, so a plain libc prototype no longer carries the implicit
// cdecl runtime convention -- it inherits the stdcall default instead. The call
// site then emits a stdcall reference (e.g. _memcpy@12, callee-cleans-stack)
// while libc/libm are compiled cdecl (_memcpy, caller-cleans-stack). lld
// fuzzy-resolves the name, but the two sides disagree on who pops the args, so
// every such call leaks stack -- fatal in a render loop.
//
// Declaring these functions __cdecl here, before picolibc's <string.h>/<math.h>
// are seen, pins the correct convention (the first declaration's CC wins). This
// is exactly what the Xbox CRT headers' explicit __cdecl did under /Gz, and
// mirrors the ExFreePool cdecl fix (commit f658e97). __SIZE_TYPE__ is used so
// the size parameter matches picolibc's size_t (unsigned int on this target).
//
// Set covers every libc/libm symbol libd3d8 actually calls (expf, memcmp,
// memcpy, memset); add to it if a future driver path introduces more.
//------------------------------------------------------------------------------
#ifndef RXDK_D3D8_CDECL_LIBC_H
#define RXDK_D3D8_CDECL_LIBC_H

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cdecl)) void *memcpy(void *, const void *, __SIZE_TYPE__);
__attribute__((cdecl)) void *memset(void *, int, __SIZE_TYPE__);
__attribute__((cdecl)) int   memcmp(const void *, const void *, __SIZE_TYPE__);
__attribute__((cdecl)) float expf(float);

#ifdef __cplusplus
}
#endif

#endif // RXDK_D3D8_CDECL_LIBC_H
