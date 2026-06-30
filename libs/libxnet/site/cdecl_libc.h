//------------------------------------------------------------------------------
// cdecl_libc.h -- force-included FIRST into every libxnet translation unit.
//
// libxnet is built with -fdefault-calling-conv=stdcall (the Xbox net stack was
// /Gz) so its vendor kernel calls (Ke/Mm/Hal/Phy/Ex + the Xc* crypto) compile
// __stdcall and bind directly to libkernel.lib -- no cdecl->stdcall facades. With
// -ffreestanding, a plain libc prototype would inherit the stdcall default instead
// of the cdecl runtime convention, so the call site emits e.g. _memcpy@12 (callee
// pops) while libc is compiled cdecl (_memcpy, caller pops) -> stack corruption.
//
// Declaring these __cdecl here, before picolibc's <string.h> is seen, pins the
// convention (first declaration wins). Mirrors libd3d8/libdsound cdecl_libc.h. The
// set is exactly the libc symbols libxnet leaves undefined; extend if more appear.
// (__alldiv is a compiler 64-bit-divide helper with a fixed ABI -- not here.)
//------------------------------------------------------------------------------
#ifndef RXDK_XNET_CDECL_LIBC_H
#define RXDK_XNET_CDECL_LIBC_H

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cdecl)) void  *memcpy(void *, const void *, __SIZE_TYPE__);
__attribute__((cdecl)) void  *memset(void *, int, __SIZE_TYPE__);
__attribute__((cdecl)) int    memcmp(const void *, const void *, __SIZE_TYPE__);
__attribute__((cdecl)) __SIZE_TYPE__ strlen(const char *);
__attribute__((cdecl)) int    strncasecmp(const char *, const char *, __SIZE_TYPE__);

#ifdef __cplusplus
}
#endif

#endif // RXDK_XNET_CDECL_LIBC_H
