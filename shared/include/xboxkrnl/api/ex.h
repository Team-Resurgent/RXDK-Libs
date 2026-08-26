/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Ex* kernel imports: the executive support layer. Pool (heap) allocation,
 * reader/writer locks, interlocked list and 64-bit primitives, exception
 * raising, the object types for the executive-managed dispatcher objects, and
 * access to the persisted EEPROM non-volatile settings and refurb info.
 */

#ifndef XBOXKRNL_API_EX_H
#define XBOXKRNL_API_EX_H

/* Acquires ReadWriteLock for exclusive (writer) access, blocking until granted.
 * Must be at PASSIVE_LEVEL. */
VOID STDCALL ExAcquireReadWriteLockExclusive(
    IN PERWLOCK ReadWriteLock);

/* Acquires ReadWriteLock for shared (reader) access, blocking until granted. */
VOID STDCALL ExAcquireReadWriteLockShared(
    IN PERWLOCK ReadWriteLock);

/* Allocates NumberOfBytes from the kernel pool. Returns NULL on failure. Free
 * with ExFreePool. Callable at <= DISPATCH_LEVEL. */
PVOID STDCALL ExAllocatePool(
    IN SIZE_T NumberOfBytes);

/* As ExAllocatePool, but stamps the allocation with a four-character Tag for
 * pool tracking. */
PVOID STDCALL ExAllocatePoolWithTag(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag);

/* The object type for event objects. */
XBAPI OBJECT_TYPE ExEventObjectType[1];

/* Atomically inserts ListEntry at the head of ListHead; returns the previous
 * first entry. Fast call, uses an implicit lock. */
PLIST_ENTRY FASTCALL ExfInterlockedInsertHeadList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry);

/* Atomically inserts ListEntry at the tail of ListHead; returns the previous
 * last entry. Fast call. */
PLIST_ENTRY FASTCALL ExfInterlockedInsertTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry);

/* Atomically removes and returns the first entry of ListHead, or NULL if empty.
 * Fast call. */
PLIST_ENTRY FASTCALL ExfInterlockedRemoveHeadList(
    IN PLIST_ENTRY ListHead);

/* Frees a block previously returned by ExAllocatePool/ExAllocatePoolWithTag. */
VOID STDCALL ExFreePool(
    IN PVOID P);

/* Initializes a reader/writer lock to the unowned state. */
VOID STDCALL ExInitializeReadWriteLock(
    IN PERWLOCK ReadWriteLock);

/* Atomically adds Increment to *Addend under Lock and returns the prior value. */
LARGE_INTEGER STDCALL ExInterlockedAddLargeInteger(
    IN OUT PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN PVOID Lock);

/* Atomically adds a 32-bit Increment to a 64-bit statistic counter. Fast call. */
VOID FASTCALL ExInterlockedAddLargeStatistic(
    IN PLARGE_INTEGER Addend,
    IN ULONG Increment);

/* Atomic 64-bit compare-and-exchange: if *Destination == *Comparand, store
 * *Exchange. Returns the prior *Destination. Fast call. */
LONGLONG FASTCALL ExInterlockedCompareExchange64(
    IN OUT LONGLONG volatile* Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comparand);

/* The object type for mutant (mutex) objects. */
XBAPI OBJECT_TYPE ExMutantObjectType[1];

/* Reads a setting from the EEPROM non-volatile store. ValueIndex selects the
 * setting; the value type is returned in *Type, the data (up to ValueLength
 * bytes) in Value, and the byte count in *ResultLength. */
NTSTATUS STDCALL ExQueryNonVolatileSetting(
    IN ULONG ValueIndex,
    OUT PULONG Type,
    OUT PVOID Value,
    IN ULONG ValueLength,
    OUT PULONG ResultLength);

/* Returns the usable size, in bytes, of the pool block at PoolBlock. */
ULONG STDCALL ExQueryPoolBlockSize(
    IN PVOID PoolBlock);

/* Raises a structured exception described by ExceptionRecord, unwinding to the
 * nearest handler. */
VOID STDCALL ExRaiseException(
    PEXCEPTION_RECORD ExceptionRecord);

/* Raises an exception carrying just the NTSTATUS Status. */
VOID STDCALL ExRaiseStatus(
    IN NTSTATUS Status);

/* Reads or (when DoWrite is TRUE) writes the console refurb-info record.
 * ValueLength sizes the RefurbInfo buffer. */
NTSTATUS STDCALL ExReadWriteRefurbInfo(
    OUT XBOX_REFURB_INFO* RefurbInfo,
    IN ULONG ValueLength,
    BOOLEAN DoWrite);

/* Releases a reader/writer lock held in either mode. */
VOID STDCALL ExReleaseReadWriteLock(
    IN PERWLOCK ReadWriteLock);

/* Writes a setting into the EEPROM non-volatile store. ValueIndex selects the
 * setting, Type/Value/ValueLength describe the new data. */
NTSTATUS STDCALL ExSaveNonVolatileSetting(
    IN ULONG ValueIndex,
    IN ULONG Type,
    IN CONST PVOID Value,
    IN ULONG ValueLength);

/* The object type for semaphore objects. */
XBAPI OBJECT_TYPE ExSemaphoreObjectType[1];

/* The object type for timer objects. */
XBAPI OBJECT_TYPE ExTimerObjectType[1];

#endif
