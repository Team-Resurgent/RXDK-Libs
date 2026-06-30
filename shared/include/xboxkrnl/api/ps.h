#ifndef XBOXKRNL_API_PS_H
#define XBOXKRNL_API_PS_H

XBAPI NTSTATUS STDCALL PsCreateSystemThread
(
    OUT PHANDLE ThreadHandle,
    OUT PHANDLE ThreadId OPTIONAL,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN BOOLEAN DebuggerThread
);

XBAPI NTSTATUS STDCALL PsCreateSystemThreadEx
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

XBAPI NTSTATUS STDCALL PsQueryStatistics
(
    IN OUT PPS_STATISTICS ProcessStatistics
);

XBAPI NTSTATUS STDCALL PsSetCreateThreadNotifyRoutine
(
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
);

XBAPI OBJECT_TYPE PsThreadObjectType[1];

#endif
