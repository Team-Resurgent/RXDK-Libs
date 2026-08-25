#ifndef XBOXKRNL_API_NT_H
#define XBOXKRNL_API_NT_H

// NtCreateFile / NtOpenFile CreateOptions flags (standard NT values). The kernel exposes the
// calls; titles pass these flags for the CreateOptions argument. Guarded on one representative
// macro so a fuller windows header can define the set instead without clashing.
#ifndef FILE_SYNCHRONOUS_IO_NONALERT
#define FILE_DIRECTORY_FILE             0x00000001
#define FILE_WRITE_THROUGH              0x00000002
#define FILE_SEQUENTIAL_ONLY            0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING  0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT       0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT    0x00000020
#define FILE_NON_DIRECTORY_FILE         0x00000040
#define FILE_CREATE_TREE_CONNECTION     0x00000080
#define FILE_COMPLETE_IF_OPLOCKED       0x00000100
#define FILE_NO_EA_KNOWLEDGE            0x00000200
#define FILE_OPEN_FOR_RECOVERY          0x00000400
#define FILE_RANDOM_ACCESS              0x00000800
#define FILE_DELETE_ON_CLOSE            0x00001000
#define FILE_OPEN_BY_FILE_ID            0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT     0x00004000
#define FILE_NO_COMPRESSION             0x00008000
#endif

NTSTATUS STDCALL NtAllocateVirtualMemory
(
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
);

NTSTATUS STDCALL NtCancelTimer
(
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL
);

NTSTATUS STDCALL NtClearEvent
(
    IN HANDLE EventHandle
);

NTSTATUS STDCALL NtClose
(
    IN HANDLE Handle
);

NTSTATUS STDCALL NtCreateDirectoryObject
(
    OUT PHANDLE DirectoryHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS STDCALL NtCreateEvent
(
    OUT PHANDLE EventHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

NTSTATUS STDCALL NtCreateFile
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

NTSTATUS STDCALL NtCreateIoCompletion
(
    OUT PHANDLE IoCompletionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG Count OPTIONAL
);

NTSTATUS STDCALL NtCreateMutant
(
    OUT PHANDLE MutantHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN BOOLEAN InitialOwner
);

NTSTATUS STDCALL NtCreateSemaphore
(
    OUT PHANDLE SemaphoreHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount
);

NTSTATUS STDCALL NtCreateTimer
(
    OUT PHANDLE TimerHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN TIMER_TYPE TimerType
);

BOOLEAN STDCALL NtDeleteFile
(
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS STDCALL NtDeviceIoControlFile
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

NTSTATUS STDCALL NtDuplicateObject
(
    IN HANDLE SourceHandle,
    OUT PHANDLE TargetHandle,
    IN ULONG Options
);

NTSTATUS STDCALL NtFlushBuffersFile
(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

NTSTATUS STDCALL NtFreeVirtualMemory
(
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
);

NTSTATUS STDCALL NtFsControlFile
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

NTSTATUS STDCALL NtOpenDirectoryObject
(
    OUT PHANDLE DirectoryHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS STDCALL NtOpenFile
(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
);

NTSTATUS STDCALL NtOpenSymbolicLinkObject
(
    OUT PHANDLE LinkHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS STDCALL NtProtectVirtualMemory
(
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG NewProtect,
    OUT PULONG OldProtect
);

NTSTATUS STDCALL NtPulseEvent
(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
);

NTSTATUS STDCALL NtQueryDirectoryFile
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

NTSTATUS STDCALL NtQueryDirectoryObject
(
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
);

NTSTATUS STDCALL NtQueryEvent
(
    IN HANDLE EventHandle,
    OUT PEVENT_BASIC_INFORMATION EventInformation
);

NTSTATUS STDCALL NtQueryFullAttributesFile
(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
);

NTSTATUS STDCALL NtQueryInformationFile
(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS STDCALL NtQueryIoCompletion
(
    IN HANDLE IoCompletionHandle,
    OUT PIO_COMPLETION_BASIC_INFORMATION IoCompletionInformation
);

NTSTATUS STDCALL NtQueryMutant
(
    IN HANDLE MutantHandle,
    OUT PMUTANT_BASIC_INFORMATION MutantInformation
);

NTSTATUS STDCALL NtQuerySemaphore
(
    IN HANDLE SemaphoreHandle,
    OUT PSEMAPHORE_BASIC_INFORMATION SemaphoreInformation
);

NTSTATUS STDCALL NtQuerySymbolicLinkObject
(
    IN HANDLE LinkHandle,
    IN OUT POBJECT_STRING LinkTarget,
    OUT PULONG ReturnedLength OPTIONAL
);

NTSTATUS STDCALL NtQueryTimer
(
    IN HANDLE TimerHandle,
    OUT PTIMER_BASIC_INFORMATION TimerInformation
);

NTSTATUS STDCALL NtQueryVirtualMemory
(
    IN PVOID BaseAddress,
    OUT PMEMORY_BASIC_INFORMATION MemoryInformation
);

NTSTATUS STDCALL NtQueryVolumeInformationFile
(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

NTSTATUS STDCALL NtQueueApcThread
(
    IN HANDLE ThreadHandle,
    IN PPS_APC_ROUTINE ApcRoutine,
    IN PVOID ApcArgument1,
    IN PVOID ApcArgument2,
    IN PVOID ApcArgument3
);

NTSTATUS STDCALL NtReadFile
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

NTSTATUS STDCALL NtReadFileScatter
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

NTSTATUS STDCALL NtReleaseMutant
(
    IN HANDLE MutantHandle,
    OUT PLONG PreviousCount OPTIONAL
);

NTSTATUS STDCALL NtReleaseSemaphore
(
    IN HANDLE SemaphoreHandle,
    IN LONG ReleaseCount,
    OUT PLONG PreviousCount OPTIONAL
);

NTSTATUS STDCALL NtRemoveIoCompletion
(
    IN HANDLE IoCompletionHandle,
    OUT PVOID *KeyContext,
    OUT PVOID *ApcContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER Timeout
);

NTSTATUS STDCALL NtResumeThread
(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
);

NTSTATUS STDCALL NtSetEvent
(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
);

NTSTATUS STDCALL NtSetInformationFile
(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS STDCALL NtSetIoCompletion
(
    IN HANDLE IoCompletionHandle,
    IN PVOID KeyContext,
    IN PVOID ApcContext,
    IN NTSTATUS IoStatus,
    IN ULONG_PTR IoStatusInformation
);

NTSTATUS STDCALL NtSetSystemTime
(
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER PreviousTime OPTIONAL
);

NTSTATUS STDCALL NtSetTimerEx
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

NTSTATUS STDCALL NtSignalAndWaitForSingleObjectEx
(
    IN HANDLE SignalHandle,
    IN HANDLE WaitHandle,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSTATUS STDCALL NtSuspendThread
(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
);

VOID STDCALL NtUserIoApcDispatcher
(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
);

NTSTATUS STDCALL NtWaitForMultipleObjectsEx
(
    IN ULONG Count,
    IN CONST HANDLE Handles[],
    IN WAIT_TYPE WaitType,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSTATUS STDCALL NtWaitForSingleObject
(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSTATUS STDCALL NtWaitForSingleObjectEx
(
    IN HANDLE Handle,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSTATUS STDCALL NtWriteFile
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

BOOLEAN STDCALL NtWriteFileGather
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

NTSTATUS STDCALL NtYieldExecution(void);

#endif
