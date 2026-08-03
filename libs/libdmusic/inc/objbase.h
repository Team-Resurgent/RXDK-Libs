#ifndef _RXDK_DMUSIC_OBJBASE_H_
#define _RXDK_DMUSIC_OBJBASE_H_
/*
 * objbase.h shim for the Xbox DirectMusic port. The DirectMusic sources
 * `#include <objbase.h>` for the base OLE/COM surface (IUnknown/IClassFactory/
 * IStream/IPersistStream + the HRESULT/GUID/STDMETHOD machinery). On Xbox that
 * surface is xobjbase.h, which site/bridge_dmusic.h already force-includes; this
 * shim shadows zig's MinGW objbase.h (which pulls rpc.h -> _mingw vadefs and
 * fails on this freestanding target) so the angle-bracket include resolves to
 * the Xbox COM headers instead. COM_NO_WINDOWS_H is set by the callers.
 */
#include <xobjbase.h>

/* CoTaskMem* / apartment helpers the loader + scripting reference. The Xbox
   retail exports live in libxapi/libkernel; declare the prototypes the callers
   need (extern-C, __stdcall) so they resolve at title link. */
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _RXDK_DMUSIC_COTASKMEM_
#define _RXDK_DMUSIC_COTASKMEM_
void   __stdcall CoTaskMemFree(void *pv);
void * __stdcall CoTaskMemAlloc(unsigned long cb);
void * __stdcall CoTaskMemRealloc(void *pv, unsigned long cb);
#endif
#ifdef __cplusplus
}
#endif

#endif /* _RXDK_DMUSIC_OBJBASE_H_ */
