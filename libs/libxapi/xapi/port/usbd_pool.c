#include "bridge_usb.h"
/*
 * Native USBD pool allocator (vendor usbd tree declares but does not define).
 *
 * Must match the cdecl linkage usbd.cpp / tree.cpp use at call sites (__attribute__((__stdcall__)) on
 * a forward decl alone does not fix stdcall/cdecl mismatches with lld-link).
 */

#include <ntos.h>

PVOID USBD_AllocateMemory(ULONG cb, ULONG Tag)
{
    (void)Tag;
    return ExAllocatePool((SIZE_T)cb);
}

VOID USBD_FreeMemory(PVOID pv)
{
    ExFreePool(pv);
}
