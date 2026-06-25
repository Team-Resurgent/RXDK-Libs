#pragma once
#define RXDK_XAPI_WINDEF_COMPAT_H


/*
 * Vendor-only calling-convention tokens. RXDK port/minilib code must use
 * __stdcall / __cdecl / __attribute__((fastcall)) directly.
 */

#ifndef FASTCALL
#define FASTCALL __attribute__((fastcall))
#endif

