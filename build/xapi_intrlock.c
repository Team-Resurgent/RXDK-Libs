/* RXDK replacement for vendor i386/intrlock.c (clang ICE) and MASM intrlock.asm. */

#include <xboxkrnl/xboxdef.h>
#include <xboxkrnl/types/common.h>

#ifndef WINAPI
#define WINAPI __stdcall
#endif

LONG WINAPI InterlockedIncrement(PLONG Addend)
{
    return (LONG)__atomic_add_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}

LONG WINAPI InterlockedDecrement(PLONG Addend)
{
    return (LONG)__atomic_sub_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}

LONG WINAPI InterlockedExchange(PLONG Target, LONG Value)
{
    return (LONG)__atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}

PVOID WINAPI InterlockedCompareExchange(PVOID *Destination, PVOID Exchange, PVOID Comperand)
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

LONG WINAPI InterlockedExchangeAdd(PLONG Addend, LONG Increment)
{
    return (LONG)__atomic_fetch_add(Addend, Increment, __ATOMIC_SEQ_CST);
}
