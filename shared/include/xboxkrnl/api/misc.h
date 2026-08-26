/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Assorted kernel imports that do not fit one Nt/Ke/Ex prefix: the Interlocked*
 * atomic family (including the S-List primitives), the Kf* IRQL raise/lower fast
 * calls, the Kd* debugger-presence flags and Ki* dispatcher internals, the Phy*
 * network PHY controls, thread termination, and the READ_PORT_BUFFER_* port-I/O
 * block reads.
 */

#ifndef XBOXKRNL_API_MISC_H
#define XBOXKRNL_API_MISC_H

/* Ends the calling system thread with ExitStatus. Never returns. */
VOID STDCALL __attribute__((noreturn)) PsTerminateSystemThread(
    IN NTSTATUS ExitStatus);

/* Atomic compare-and-exchange: if *Destination == Comparand, store ExChange.
 * Returns the prior *Destination. Fast call. */
LONG FASTCALL InterlockedCompareExchange(
    IN OUT LONG volatile* Destination,
    IN LONG ExChange,
    IN LONG Comparand);

/* Atomically decrements *Addend and returns the new value. Fast call. */
LONG FASTCALL InterlockedDecrement(
    IN OUT LONG volatile* Addend);

/* Atomically sets *Target to Value and returns the prior value. Fast call. */
LONG FASTCALL InterlockedExchange(
    IN OUT LONG volatile* Target,
    IN LONG Value);

/* Atomically adds Increment to *Addend and returns the prior value. Fast call. */
LONG FASTCALL InterlockedExchangeAdd(
    IN OUT LONG volatile* Addend,
    IN LONG Increment);

/* Atomically empties an interlocked singly-linked list, returning its former
 * first entry. Fast call. */
PSINGLE_LIST_ENTRY FASTCALL InterlockedFlushSList(
    IN PSLIST_HEADER ListHead);

/* Atomically increments *Addend and returns the new value. Fast call. */
LONG FASTCALL InterlockedIncrement(
    IN OUT LONG volatile* Addend);

/* Atomically pops the first entry off an interlocked S-List, or NULL if empty.
 * Fast call. */
PSINGLE_LIST_ENTRY FASTCALL InterlockedPopEntrySList(
    IN PSLIST_HEADER ListHead);

/* Atomically pushes ListEntry onto an interlocked S-List; returns the prior
 * first entry. Fast call. */
PSINGLE_LIST_ENTRY FASTCALL InterlockedPushEntrySList(
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry);

/* TRUE while a kernel debugger is attached and enabled. */
XBAPI BOOLEAN KdDebuggerEnabled;

/* TRUE when no kernel debugger is present. */
XBAPI BOOLEAN KdDebuggerNotPresent;

/* Lowers the current IRQL to NewIrql, delivering any pending lower-IRQL work.
 * Fast call. */
VOID FASTCALL KfLowerIrql(
    IN KIRQL NewIrql);

/* Raises the current IRQL to NewIrql and returns the previous IRQL to restore
 * later with KfLowerIrql. Fast call. */
KIRQL FASTCALL KfRaiseIrql(
    IN KIRQL NewIrql);

/* Bug-check parameter array captured at the last fatal kernel error. */
XBAPI ULONG KiBugCheckData[];

/* Releases the dispatcher database lock, restoring IRQL to OldIrql. Internal
 * scheduler primitive. Fast call. */
VOID FASTCALL KiUnlockDispatcherDatabase(
    IN KIRQL OldIrql);

/* Returns the current Ethernet PHY link state; when update is TRUE the state is
 * re-read from hardware first. */
DWORD STDCALL PhyGetLinkState(
    BOOLEAN update);

/* Initializes the Ethernet PHY. forceReset forces a hardware reset; param
 * carries optional configuration. */
NTSTATUS STDCALL PhyInitialize(
    BOOLEAN forceReset,
    PVOID param OPTIONAL);

/* Reads Count consecutive UCHARs from I/O Port into Buffer. */
VOID STDCALL READ_PORT_BUFFER_UCHAR(
    IN PUCHAR Port,
    OUT PUCHAR Buffer,
    IN ULONG Count);

/* Reads Count consecutive ULONGs from I/O Port into Buffer. */
VOID STDCALL READ_PORT_BUFFER_ULONG(
    IN PULONG Port,
    OUT PULONG Buffer,
    IN ULONG Count);

/* Reads Count consecutive USHORTs from I/O Port into Buffer. */
VOID STDCALL READ_PORT_BUFFER_USHORT(
    IN PUSHORT Port,
    OUT PUSHORT Buffer,
    IN ULONG Count);

#endif
