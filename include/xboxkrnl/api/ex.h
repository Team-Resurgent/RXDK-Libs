#ifndef XBOXKRNL_API_EX_H
#define XBOXKRNL_API_EX_H

XBAPI VOID NTAPI ExAcquireReadWriteLockExclusive
(
    IN PERWLOCK ReadWriteLock
);

XBAPI VOID NTAPI ExAcquireReadWriteLockShared
(
    IN PERWLOCK ReadWriteLock
);

XBAPI PVOID NTAPI ExAllocatePool
(
    IN SIZE_T NumberOfBytes
);

XBAPI PVOID NTAPI ExAllocatePoolWithTag
(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
);

RXDK_XBOXKRNL_OBJECT_TYPE(ExEventObjectType);

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

XBAPI VOID NTAPI ExFreePool
(
    IN PVOID P
);

XBAPI VOID NTAPI ExInitializeReadWriteLock
(
    IN PERWLOCK ReadWriteLock
);

XBAPI LARGE_INTEGER NTAPI ExInterlockedAddLargeInteger
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

RXDK_XBOXKRNL_OBJECT_TYPE(ExMutantObjectType);

XBAPI NTSTATUS NTAPI ExQueryNonVolatileSetting
(
    IN ULONG ValueIndex,
    OUT PULONG Type,
    OUT PVOID Value,
    IN ULONG ValueLength,
    OUT PULONG ResultLength
);

XBAPI ULONG NTAPI ExQueryPoolBlockSize
(
    IN PVOID PoolBlock
);

XBAPI VOID NTAPI ExRaiseException
(
    PEXCEPTION_RECORD ExceptionRecord
);

XBAPI VOID NTAPI ExRaiseStatus
(
    IN NTSTATUS Status
);

XBAPI NTSTATUS NTAPI ExReadWriteRefurbInfo
(
    OUT XBOX_REFURB_INFO *RefurbInfo,
    IN ULONG ValueLength,
    BOOLEAN DoWrite
);

XBAPI VOID NTAPI ExReleaseReadWriteLock
(
    IN PERWLOCK ReadWriteLock
);

XBAPI NTSTATUS NTAPI ExSaveNonVolatileSetting
(
    IN ULONG ValueIndex,
    IN ULONG Type,
    IN CONST PVOID Value,
    IN ULONG ValueLength
);

RXDK_XBOXKRNL_OBJECT_TYPE(ExSemaphoreObjectType);

RXDK_XBOXKRNL_OBJECT_TYPE(ExTimerObjectType);

#endif
