/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Private interface declaration for the loader's IMA (Interactive Music
 * Architecture) legacy mode, which loads content authored for the older
 * DirectMusic object model.
 */

#ifndef __IMA_H_
#define __IMA_H_


#ifdef XBOX
#include <xtl.h>
#else // XBOX
#include <windows.h>
#endif // XBOX


#define COM_NO_WINDOWS_H
#include <objbase.h>

#ifdef __cplusplus
extern "C" {
#endif


#undef  INTERFACE
#define INTERFACE  IDirectMusicIMA
DECLARE_INTERFACE_(IDirectMusicIMA, IUnknown)
{
	/* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	/* IDirectMusicIMA */
	STDMETHOD(LegacyCaching)		(THIS_ BOOL fEnable) PURE;
};

DEFINE_GUID(IID_IDirectMusicIMA,0xd2ac28b3, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif /* #ifndef __IMA_H_ */
