/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * GUID storage for the Kernel Streaming headers: defines INITGUID and the
 * DEFINE_GUIDEX machinery so the KS GUIDs get real storage in the translation
 * unit that includes this.
 */

#define INITGUID
#include <guiddef.h>

#if defined( DEFINE_GUIDEX )
    #undef DEFINE_GUIDEX
#endif
#define DEFINE_GUIDEX(name) EXTERN_C const CDECL GUID __declspec(selectany) name = { STATICGUIDOF(name) }

#ifndef STATICGUIDOF
    #define STATICGUIDOF(guid) STATIC_##guid
#endif // !defined(STATICGUIDOF)

#if !defined( DEFINE_WAVEFORMATEX_GUID )
#define DEFINE_WAVEFORMATEX_GUID(x) (USHORT)(x), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
#endif
