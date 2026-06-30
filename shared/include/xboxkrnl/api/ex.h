#ifndef XBOXKRNL_API_EX_H
#define XBOXKRNL_API_EX_H

VOID STDCALL ExAcquireReadWriteLockExclusive
(
    IN PERWLOCK ReadWriteLock
);

VOID STDCALL ExAcquireReadWriteLockShared
(
    IN PERWLOCK ReadWriteLock
);

PVOID STDCALL ExAllocatePool
(
    IN SIZE_T NumberOfBytes
);

PVOID STDCALL ExAllocatePoolWithTag
(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
);

XBAPI OBJECT_TYPE ExEventObjectType[1];

PLIST_ENTRY FASTCALL ExfInterlockedInsertHeadList
(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
);

PLIST_ENTRY FASTCALL ExfInterlockedInsertTailList
(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
);

PLIST_ENTRY FASTCALL ExfInterlockedRemoveHeadList
(
    IN PLIST_ENTRY ListHead
);

VOID STDCALL ExFreePool
(
    IN PVOID P
);

VOID STDCALL ExInitializeReadWriteLock
(
    IN PERWLOCK ReadWriteLock
);

LARGE_INTEGER STDCALL ExInterlockedAddLargeInteger
(
    IN OUT PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN PVOID Lock
);

VOID FASTCALL ExInterlockedAddLargeStatistic
(
    IN PLARGE_INTEGER Addend,
    IN ULONG Increment
);

LONGLONG FASTCALL ExInterlockedCompareExchange64
(
    IN OUT LONGLONG volatile *Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comparand
);

XBAPI OBJECT_TYPE ExMutantObjectType[1];

NTSTATUS STDCALL ExQueryNonVolatileSetting
(
    IN ULONG ValueIndex,
    OUT PULONG Type,
    OUT PVOID Value,
    IN ULONG ValueLength,
    OUT PULONG ResultLength
);

ULONG STDCALL ExQueryPoolBlockSize
(
    IN PVOID PoolBlock
);

VOID STDCALL ExRaiseException
(
    PEXCEPTION_RECORD ExceptionRecord
);

VOID STDCALL ExRaiseStatus
(
    IN NTSTATUS Status
);

NTSTATUS STDCALL ExReadWriteRefurbInfo
(
    OUT XBOX_REFURB_INFO *RefurbInfo,
    IN ULONG ValueLength,
    BOOLEAN DoWrite
);

VOID STDCALL ExReleaseReadWriteLock
(
    IN PERWLOCK ReadWriteLock
);

NTSTATUS STDCALL ExSaveNonVolatileSetting
(
    IN ULONG ValueIndex,
    IN ULONG Type,
    IN CONST PVOID Value,
    IN ULONG ValueLength
);

XBAPI OBJECT_TYPE ExSemaphoreObjectType[1];

XBAPI OBJECT_TYPE ExTimerObjectType[1];

#endif
