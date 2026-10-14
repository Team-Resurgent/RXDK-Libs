#include "bridge_k32.h"
/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Floating-point state save/restore for DPCs: XSaveFloatingPointStateForDpc /
 * XRestoreFloatingPointStateForDpc. A DPC that wants to use floating point (for
 * example the voice-chat stream callbacks) brackets it with these calls, which
 * save and restore the x87/MMX state via the kernel's KeSaveFloatingPointState /
 * KeRestoreFloatingPointState.
 *
 * The Xbox is single-processor and DPCs are serialized, so a single static save
 * area (paired save -> restore, never nested) is sufficient - which is why these
 * routines take no context parameter.
 */

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

/*
 * RxdkRunDpcFpuSafe: run a DPC routine with the FPU/SSE state saved around it.
 * Every DPC the SDK libraries register goes through this (RXDK_FPU_SAFE_DPC in
 * build/generated/rxdk_fpu_dpc.h). A DPC runs on top of whatever thread it
 * interrupted, and the kernel does not save that thread's floating-point state
 * for it, so any x87/SSE use in the routine -- its own float math, an xmm
 * memcpy, a compiler-chosen xmm copy, a title callback such as D3D's vertical
 * blank callback -- would overwrite the interrupted thread's registers or
 * bugcheck with ATTEMPTED_FPU_USE_FROM_DPC. KeSaveFloatingPointState saves the
 * SSE registers as well as the x87 state (checked on the console's kernel).
 * The save area is on the stack, so nested routines each keep their own.
 */
VOID
WINAPI
RxdkRunDpcFpuSafe(
    PKDEFERRED_ROUTINE Routine,
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    KFLOATING_SAVE Save;
    NTSTATUS Status = KeSaveFloatingPointState(&Save);

    Routine(Dpc, DeferredContext, SystemArgument1, SystemArgument2);
    if (NT_SUCCESS(Status)) {
        KeRestoreFloatingPointState(&Save);
    }
}
