#ifndef XBOXKRNL_API_MISC_H
#define XBOXKRNL_API_MISC_H

XBAPI VOID STDCALL __attribute__ ((noreturn)) PsTerminateSystemThread
(
    IN NTSTATUS ExitStatus
);

XBAPI LONG FASTCALL InterlockedCompareExchange
(
    IN OUT LONG volatile *Destination,
    IN LONG ExChange,
    IN LONG Comparand
);

XBAPI LONG FASTCALL InterlockedDecrement
(
    IN OUT LONG volatile *Addend
);

XBAPI LONG FASTCALL InterlockedExchange
(
    IN OUT LONG volatile *Target,
    IN LONG Value
);

XBAPI LONG FASTCALL InterlockedExchangeAdd
(
    IN OUT LONG volatile *Addend,
    IN LONG Increment
);

XBAPI PSINGLE_LIST_ENTRY FASTCALL InterlockedFlushSList
(
    IN PSLIST_HEADER ListHead
);

XBAPI LONG FASTCALL InterlockedIncrement
(
    IN OUT LONG volatile *Addend
);

XBAPI PSINGLE_LIST_ENTRY FASTCALL InterlockedPopEntrySList
(
    IN PSLIST_HEADER ListHead
);

XBAPI PSINGLE_LIST_ENTRY FASTCALL InterlockedPushEntrySList
(
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
);

XBAPI BOOLEAN KdDebuggerEnabled;

XBAPI BOOLEAN KdDebuggerNotPresent;

XBAPI VOID FASTCALL KfLowerIrql
(
    IN KIRQL NewIrql
);

XBAPI KIRQL FASTCALL KfRaiseIrql
(
    IN KIRQL NewIrql
);

XBAPI ULONG KiBugCheckData[];

XBAPI VOID FASTCALL KiUnlockDispatcherDatabase
(
    IN KIRQL OldIrql
);

XBAPI DWORD STDCALL PhyGetLinkState
(
    BOOLEAN update
);

XBAPI NTSTATUS STDCALL PhyInitialize
(
    BOOLEAN forceReset,
    PVOID param OPTIONAL
);

XBAPI VOID STDCALL READ_PORT_BUFFER_UCHAR
(
    IN PUCHAR Port,
    OUT PUCHAR Buffer,
    IN ULONG Count
);

XBAPI VOID STDCALL READ_PORT_BUFFER_ULONG
(
    IN PULONG Port,
    OUT PULONG Buffer,
    IN ULONG Count
);

XBAPI VOID STDCALL READ_PORT_BUFFER_USHORT
(
    IN PUSHORT Port,
    OUT PUSHORT Buffer,
    IN ULONG Count
);

#endif
