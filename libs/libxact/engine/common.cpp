
#include "stdafx.h"

#include "common.h"

//
// RXDK: single definition of the XACT global operator new/delete (routed through
// the memory manager, XactMemAlloc/XactMemFree). The leak defined these `static`
// in common.h -- one copy per TU -- but clang rejects a static/inline global
// replacement operator new/delete, so the one definition lives here (the "common"
// TU) and every other engine TU uses the implicitly-declared global operators.
//

void *__cdecl operator new(size_t cbBuffer)
{
    return XactMemAlloc(cbBuffer, FALSE);
}

void *__cdecl operator new[](size_t cbBuffer)
{
    return XactMemAlloc(cbBuffer, FALSE);
}

void __cdecl operator delete(void *pvBuffer)
{
    XactMemFree(pvBuffer);
}

void __cdecl operator delete[](void *pvBuffer)
{
    XactMemFree(pvBuffer);
}
