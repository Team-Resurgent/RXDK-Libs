#ifndef XBOXKRNL_API_NT_H
#define XBOXKRNL_API_NT_H

XBAPI NTSTATUS STDCALL NtAllocateVirtualMemory
(
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
);

XBAPI NTSTATUS STDCALL NtCancelTimer
(
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL
);

XBAPI NTSTATUS STDCALL NtClearEvent
(
    IN HANDLE EventHandle
);

XBAPI NTSTATUS STDCALL NtClose
(
    IN HANDLE Handle
);

XBAPI NTSTATUS STDCALL NtCreateDirectoryObject
(
    OUT PHANDLE DirectoryHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

XBAPI NTSTATUS STDCALL NtCreateEvent
(
    OUT PHANDLE EventHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

XBAPI NTSTATUS STDCALL NtCreateFile
(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions
);

XBAPI NTSTATUS STDCALL NtCreateIoCompletion
(
    OUT PHANDLE IoCompletionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG Count OPTIONAL
);

XBAPI NTSTATUS STDCALL NtCreateMutant
(
    OUT PHANDLE MutantHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN BOOLEAN InitialOwner
);

XBAPI NTSTATUS STDCALL NtCreateSemaphore
(
    OUT PHANDLE SemaphoreHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount
);

XBAPI NTSTATUS STDCALL NtCreateTimer
(
    OUT PHANDLE TimerHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN TIMER_TYPE TimerType
);

XBAPI BOOLEAN STDCALL NtDeleteFile
(
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

XBAPI NTSTATUS STDCALL NtDeviceIoControlFile
(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
);

XBAPI NTSTATUS STDCALL NtDuplicateObject
(
    IN HANDLE SourceHandle,
    OUT PHANDLE TargetHandle,
    IN ULONG Options
);

XBAPI NTSTATUS STDCALL NtFlushBuffersFile
(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

XBAPI NTSTATUS STDCALL NtFreeVirtualMemory
(
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
);

XBAPI NTSTATUS STDCALL NtFsControlFile
(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
);

XBAPI NTSTATUS STDCALL NtOpenDirectoryObject
(
    OUT PHANDLE DirectoryHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

XBAPI NTSTATUS STDCALL NtOpenFile
(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
);

XBAPI NTSTATUS STDCALL NtOpenSymbolicLinkObject
(
    OUT PHANDLE LinkHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

XBAPI NTSTATUS STDCALL NtProtectVirtualMemory
(
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG NewProtect,
    OUT PULONG OldProtect
);

XBAPI NTSTATUS STDCALL NtPulseEvent
(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
);

XBAPI NTSTATUS STDCALL NtQueryDirectoryFile
(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN POBJECT_STRING FileName OPTIONAL,
    IN BOOLEAN RestartScan
);

XBAPI NTSTATUS STDCALL NtQueryDirectoryObject
(
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
);

XBAPI NTSTATUS STDCALL NtQueryEvent
(
    IN HANDLE EventHandle,
    OUT PEVENT_BASIC_INFORMATION EventInformation
);

XBAPI NTSTATUS STDCALL NtQueryFullAttributesFile
(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
);

XBAPI NTSTATUS STDCALL NtQueryInformationFile
(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);

XBAPI NTSTATUS STDCALL NtQueryIoCompletion
(
    IN HANDLE IoCompletionHandle,
    OUT PIO_COMPLETION_BASIC_INFORMATION IoCompletionInformation
);

XBAPI NTSTATUS STDCALL NtQueryMutant
(
    IN HANDLE MutantHandle,
    OUT PMUTANT_BASIC_INFORMATION MutantInformation
);

XBAPI NTSTATUS STDCALL NtQuerySemaphore
(
    IN HANDLE SemaphoreHandle,
    OUT PSEMAPHORE_BASIC_INFORMATION SemaphoreInformation
);

XBAPI NTSTATUS STDCALL NtQuerySymbolicLinkObject
(
    IN HANDLE LinkHandle,
    IN OUT POBJECT_STRING LinkTarget,
    OUT PULONG ReturnedLength OPTIONAL
);

XBAPI NTSTATUS STDCALL NtQueryTimer
(
    IN HANDLE TimerHandle,
    OUT PTIMER_BASIC_INFORMATION TimerInformation
);

XBAPI NTSTATUS STDCALL NtQueryVirtualMemory
(
    IN PVOID BaseAddress,
    OUT PMEMORY_BASIC_INFORMATION MemoryInformation
);

XBAPI NTSTATUS STDCALL NtQueryVolumeInformationFile
(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

XBAPI NTSTATUS STDCALL NtQueueApcThread
(
    IN HANDLE ThreadHandle,
    IN PPS_APC_ROUTINE ApcRoutine,
    IN PVOID ApcArgument1,
    IN PVOID ApcArgument2,
    IN PVOID ApcArgument3
);

XBAPI NTSTATUS STDCALL NtReadFile
(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL
);

XBAPI NTSTATUS STDCALL NtReadFileScatter
(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL
);

XBAPI NTSTATUS STDCALL NtReleaseMutant
(
    IN HANDLE MutantHandle,
    OUT PLONG PreviousCount OPTIONAL
);

XBAPI NTSTATUS STDCALL NtReleaseSemaphore
(
    IN HANDLE SemaphoreHandle,
    IN LONG ReleaseCount,
    OUT PLONG PreviousCount OPTIONAL
);

XBAPI NTSTATUS STDCALL NtRemoveIoCompletion
(
    IN HANDLE IoCompletionHandle,
    OUT PVOID *KeyContext,
    OUT PVOID *ApcContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER Timeout
);

XBAPI NTSTATUS STDCALL NtResumeThread
(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
);

XBAPI NTSTATUS STDCALL NtSetEvent
(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
);

XBAPI NTSTATUS STDCALL NtSetInformationFile
(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);

XBAPI NTSTATUS STDCALL NtSetIoCompletion
(
    IN HANDLE IoCompletionHandle,
    IN PVOID KeyContext,
    IN PVOID ApcContext,
    IN NTSTATUS IoStatus,
    IN ULONG_PTR IoStatusInformation
);

XBAPI NTSTATUS STDCALL NtSetSystemTime
(
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER PreviousTime OPTIONAL
);

XBAPI NTSTATUS STDCALL NtSetTimerEx
(
    IN HANDLE TimerHandle,
    IN PLARGE_INTEGER DueTime,
    IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
    IN KPROCESSOR_MODE ApcMode,
    IN PVOID TimerContext OPTIONAL,
    IN BOOLEAN ResumeTimer,
    IN LONG Period OPTIONAL,
    OUT PBOOLEAN PreviousState OPTIONAL
);

XBAPI NTSTATUS STDCALL NtSignalAndWaitForSingleObjectEx
(
    IN HANDLE SignalHandle,
    IN HANDLE WaitHandle,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

XBAPI NTSTATUS STDCALL NtSuspendThread
(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
);

XBAPI VOID STDCALL NtUserIoApcDispatcher
(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
);

XBAPI NTSTATUS STDCALL NtWaitForMultipleObjectsEx
(
    IN ULONG Count,
    IN CONST HANDLE Handles[],
    IN WAIT_TYPE WaitType,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

XBAPI NTSTATUS STDCALL NtWaitForSingleObject
(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

XBAPI NTSTATUS STDCALL NtWaitForSingleObjectEx
(
    IN HANDLE Handle,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

XBAPI NTSTATUS STDCALL NtWriteFile
(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL
);

XBAPI BOOLEAN STDCALL NtWriteFileGather
(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL
);

XBAPI NTSTATUS STDCALL NtYieldExecution(void);

#endif
