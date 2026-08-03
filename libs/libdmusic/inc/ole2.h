#ifndef _RXDK_DMUSIC_OLE2_H_
#define _RXDK_DMUSIC_OLE2_H_
/*
 * ole2.h shim for the Xbox DirectMusic port. The leak's ole2.h is the full OLE2
 * umbrella (objbase + oleauto + oleidl + mac shims). shared/debug.h -- pulled by
 * every component -- includes it under XBOX. On Xbox the base COM surface is
 * xobjbase.h (force-included by the bridge); this shim resolves <ole2.h> to that
 * base plus the OLE-Automation types the scripting/loader paths name (BSTR,
 * VARIANT, SAFEARRAY and the Sys.. / Variant.. helpers), shadowing MinGW ole2.h.
 */
#include <objbase.h>
#include <oleauto.h>

#endif /* _RXDK_DMUSIC_OLE2_H_ */
