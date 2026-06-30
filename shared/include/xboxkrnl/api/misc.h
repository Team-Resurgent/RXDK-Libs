#ifndef XBOXKRNL_API_MISC_H
#define XBOXKRNL_API_MISC_H

VOID STDCALL __attribute__ ((noreturn)) PsTerminateSystemThread
(
    IN NTSTATUS ExitStatus
);

LONG FASTCALL InterlockedCompareExchange
(
    IN OUT LONG volatile *Destination,
    IN LONG ExChange,
    IN LONG Comparand
);

LONG FASTCALL InterlockedDecrement
(
    IN OUT LONG volatile *Addend
);

LONG FASTCALL InterlockedExchange
(
    IN OUT LONG volatile *Target,
    IN LONG Value
);

LONG FASTCALL InterlockedExchangeAdd
(
    IN OUT LONG volatile *Addend,
    IN LONG Increment
);

PSINGLE_LIST_ENTRY FASTCALL InterlockedFlushSList
(
    IN PSLIST_HEADER ListHead
);

LONG FASTCALL InterlockedIncrement
(
    IN OUT LONG volatile *Addend
);

PSINGLE_LIST_ENTRY FASTCALL InterlockedPopEntrySList
(
    IN PSLIST_HEADER ListHead
);

PSINGLE_LIST_ENTRY FASTCALL InterlockedPushEntrySList
(
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
);

XBAPI BOOLEAN KdDebuggerEnabled;

XBAPI BOOLEAN KdDebuggerNotPresent;

VOID FASTCALL KfLowerIrql
(
    IN KIRQL NewIrql
);

KIRQL FASTCALL KfRaiseIrql
(
    IN KIRQL NewIrql
);

XBAPI ULONG KiBugCheckData[];

VOID FASTCALL KiUnlockDispatcherDatabase
(
    IN KIRQL OldIrql
);

DWORD STDCALL PhyGetLinkState
(
    BOOLEAN update
);

NTSTATUS STDCALL PhyInitialize
(
    BOOLEAN forceReset,
    PVOID param OPTIONAL
);

VOID STDCALL READ_PORT_BUFFER_UCHAR
(
    IN PUCHAR Port,
    OUT PUCHAR Buffer,
    IN ULONG Count
);

VOID STDCALL READ_PORT_BUFFER_ULONG
(
    IN PULONG Port,
    OUT PULONG Buffer,
    IN ULONG Count
);

VOID STDCALL READ_PORT_BUFFER_USHORT
(
    IN PUSHORT Port,
    OUT PUSHORT Buffer,
    IN ULONG Count
);

#endif
