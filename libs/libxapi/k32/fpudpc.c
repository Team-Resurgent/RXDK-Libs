#include "bridge_k32.h"
/*++

Module Name:

    fpudpc.c

Abstract:

    RXDK 5849 uplift: XSaveFloatingPointStateForDpc / XRestoreFloatingPointStateForDpc.
    These xapilib exports are XDK-5849 additions with no source in the leak: a DPC
    that wants to use floating point (e.g. the voice-chat stream callbacks) brackets
    it with save/restore of the x87/MMX state via the kernel's
    KeSaveFloatingPointState / KeRestoreFloatingPointState.

    The Xbox is single-processor and DPCs are serialized, so a single static
    save area (paired save -> restore, never nested) is sufficient -- which is
    why the 5849 API takes no context parameter at all.

--*/

#include "basedll.h"

static KFLOATING_SAVE XapipDpcFloatingPointSave;

XBOXAPI
VOID
WINAPI
XSaveFloatingPointStateForDpc(
    VOID
    )
{
    KeSaveFloatingPointState(&XapipDpcFloatingPointSave);
}

XBOXAPI
VOID
WINAPI
XRestoreFloatingPointStateForDpc(
    VOID
    )
{
    KeRestoreFloatingPointState(&XapipDpcFloatingPointSave);
}
