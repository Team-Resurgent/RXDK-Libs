/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Storage + setters for the libc I/O/process hooks (see xbox/libc_hooks.h).
 * libc reads the globals directly; the app installs handlers via the setters.
 */

#include "xbox/libc_hooks.h"

rxdk_stdin_fn __rxdk_stdin_hook = NULL;
rxdk_output_fn __rxdk_output_hook = NULL;
rxdk_exec_fn __rxdk_exec_hook = NULL;

void rxdk_set_stdin_handler(rxdk_stdin_fn fn)
{
    __rxdk_stdin_hook = fn;
}

void rxdk_set_output_handler(rxdk_output_fn fn)
{
    __rxdk_output_hook = fn;
}

void rxdk_set_exec_handler(rxdk_exec_fn fn)
{
    __rxdk_exec_hook = fn;
}
