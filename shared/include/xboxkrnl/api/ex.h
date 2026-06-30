#ifndef XBOXKRNL_API_EX_H
#define XBOXKRNL_API_EX_H

XBAPI VOID STDCALL ExAcquireReadWriteLockExclusive
(
    IN PERWLOCK ReadWriteLock
);

XBAPI VOID STDCALL ExAcquireReadWriteLockShared
(
    IN PERWLOCK ReadWriteLock
);

XBAPI PVOID STDCALL ExAllocatePool
(
    IN SIZE_T NumberOfBytes
);

XBAPI PVOID STDCALL ExAllocatePoolWithTag
(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
);

XBAPI OBJECT_TYPE ExEventObjectType[1];

XBAPI PLIST_ENTRY FASTCALL ExfInterlockedInsertHeadList
(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
);

XBAPI PLIST_ENTRY FASTCALL ExfInterlockedInsertTailList
(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
);

XBAPI PLIST_ENTRY FASTCALL ExfInterlockedRemoveHeadList
(
    IN PLIST_ENTRY ListHead
);

XBAPI VOID STDCALL ExFreePool
(
    IN PVOID P
);

XBAPI VOID STDCALL ExInitializeReadWriteLock
(
    IN PERWLOCK ReadWriteLock
);

XBAPI LARGE_INTEGER STDCALL ExInterlockedAddLargeInteger
(
    IN OUT PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN PVOID Lock
);

XBAPI VOID FASTCALL ExInterlockedAddLargeStatistic
(
    IN PLARGE_INTEGER Addend,
    IN ULONG Increment
);

XBAPI LONGLONG FASTCALL ExInterlockedCompareExchange64
(
    IN OUT LONGLONG volatile *Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comparand
);

XBAPI OBJECT_TYPE ExMutantObjectType[1];

XBAPI NTSTATUS STDCALL ExQueryNonVolatileSetting
(
    IN ULONG ValueIndex,
    OUT PULONG Type,
    OUT PVOID Value,
    IN ULONG ValueLength,
    OUT PULONG ResultLength
);

XBAPI ULONG STDCALL ExQueryPoolBlockSize
(
    IN PVOID PoolBlock
);

XBAPI VOID STDCALL ExRaiseException
(
    PEXCEPTION_RECORD ExceptionRecord
);

XBAPI VOID STDCALL ExRaiseStatus
(
    IN NTSTATUS Status
);

XBAPI NTSTATUS STDCALL ExReadWriteRefurbInfo
(
    OUT XBOX_REFURB_INFO *RefurbInfo,
    IN ULONG ValueLength,
    BOOLEAN DoWrite
);

XBAPI VOID STDCALL ExReleaseReadWriteLock
(
    IN PERWLOCK ReadWriteLock
);

XBAPI NTSTATUS STDCALL ExSaveNonVolatileSetting
(
    IN ULONG ValueIndex,
    IN ULONG Type,
    IN CONST PVOID Value,
    IN ULONG ValueLength
);

XBAPI OBJECT_TYPE ExSemaphoreObjectType[1];

XBAPI OBJECT_TYPE ExTimerObjectType[1];

#endif
