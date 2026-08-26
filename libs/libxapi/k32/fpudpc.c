#include "bridge_k32.h"
/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
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
