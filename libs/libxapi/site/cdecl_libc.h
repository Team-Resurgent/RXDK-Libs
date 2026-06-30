//------------------------------------------------------------------------------
// cdecl_libc.h -- force-included FIRST into the libxapi USB driver TUs (ohcd + the
// usbd/hub/mu/xid C++), which build -fdefault-calling-conv=stdcall so bare vendor
// kernel decls bind direct to libkernel.lib (no cdecl->stdcall facades).
//
// -ffreestanding strips clang's builtin libc recognition, so a plain libc prototype
// would inherit the stdcall default instead of cdecl: _memcpy@12 (callee pops) vs
// libc's cdecl _memcpy (caller pops) -> corruption. Declaring these __cdecl here,
// before picolibc's <string.h>, pins the convention. Mirrors the libd3d8/libdsound/
// libxnet cdecl_libc.h. (sprintf is variadic -> stays cdecl under the flag anyway.)
//------------------------------------------------------------------------------
#ifndef RXDK_XAPI_USB_CDECL_LIBC_H
#define RXDK_XAPI_USB_CDECL_LIBC_H

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cdecl)) void *memcpy(void *, const void *, __SIZE_TYPE__);
__attribute__((cdecl)) void *memset(void *, int, __SIZE_TYPE__);
__attribute__((cdecl)) int   memcmp(const void *, const void *, __SIZE_TYPE__);

#ifdef __cplusplus
}
#endif

#endif // RXDK_XAPI_USB_CDECL_LIBC_H
