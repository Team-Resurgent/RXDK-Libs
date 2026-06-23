#ifndef XBOXKRNL_API_PS_H
#define XBOXKRNL_API_PS_H

XBAPI NTSTATUS NTAPI PsCreateSystemThread
(
    OUT PHANDLE ThreadHandle,
    OUT PHANDLE ThreadId OPTIONAL,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN BOOLEAN DebuggerThread
);

XBAPI NTSTATUS NTAPI PsCreateSystemThreadEx
(
    OUT PHANDLE ThreadHandle,
    IN SIZE_T ThreadExtensionSize,
    IN SIZE_T KernelStackSize,
    IN SIZE_T TlsDataSize,
    OUT PHANDLE ThreadId OPTIONAL,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN BOOLEAN CreateSuspended,
    IN BOOLEAN DebuggerThread,
    IN PKSYSTEM_ROUTINE SystemRoutine OPTIONAL
);

XBAPI NTSTATUS NTAPI PsQueryStatistics
(
    IN OUT PPS_STATISTICS ProcessStatistics
);

XBAPI NTSTATUS NTAPI PsSetCreateThreadNotifyRoutine
(
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
);

XBAPI OBJECT_TYPE PsThreadObjectType;

#endif
