/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Interlocked primitives (InterlockedIncrement / Decrement / Exchange /
 * CompareExchange / ExchangeAdd) implemented over the Clang __atomic builtins
 * with sequentially-consistent ordering, replacing the original i386 assembly.
 */

#include "bridge_k32.h"

#include <xboxkrnl/xboxdef.h>
#include <xboxkrnl/types/common.h>

LONG __stdcall InterlockedIncrement(PLONG Addend)
{
    return (LONG)__atomic_add_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}

LONG __stdcall InterlockedDecrement(PLONG Addend)
{
    return (LONG)__atomic_sub_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}

LONG __stdcall InterlockedExchange(PLONG Target, LONG Value)
{
    return (LONG)__atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}

PVOID __stdcall InterlockedCompareExchange(PVOID *Destination, PVOID Exchange, PVOID Comperand)
{
    PVOID expected = Comperand;
    (void)__atomic_compare_exchange_n(
        Destination,
        &expected,
        Exchange,
        0,
        __ATOMIC_SEQ_CST,
        __ATOMIC_SEQ_CST);
    return expected;
}

LONG __stdcall InterlockedExchangeAdd(PLONG Addend, LONG Increment)
{
    return (LONG)__atomic_fetch_add(Addend, Increment, __ATOMIC_SEQ_CST);
}
