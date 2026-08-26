/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Nt* kernel imports: the system-call surface. Handle-based access to files,
 * directories, virtual memory, and the named synchronization objects (events,
 * mutants, semaphores, timers, I/O completion). These follow the NT convention:
 * every routine returns an NTSTATUS, handles are opaque, optional out-parameters
 * report prior state, and Timeout arguments are 100 ns LARGE_INTEGERs (negative
 * = relative, NULL = infinite). File open/query flags come from the FILE_* set
 * below.
 */

#ifndef XBOXKRNL_API_NT_H
#define XBOXKRNL_API_NT_H

// NtCreateFile / NtOpenFile CreateOptions flags (standard NT values). The kernel exposes the
// calls; titles pass these flags for the CreateOptions argument. Guarded on one representative
// macro so a fuller windows header can define the set instead without clashing.
#ifndef FILE_SYNCHRONOUS_IO_NONALERT
#define FILE_DIRECTORY_FILE 0x00000001
#define FILE_WRITE_THROUGH 0x00000002
#define FILE_SEQUENTIAL_ONLY 0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING 0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT 0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
#define FILE_NON_DIRECTORY_FILE 0x00000040
#define FILE_CREATE_TREE_CONNECTION 0x00000080
#define FILE_COMPLETE_IF_OPLOCKED 0x00000100
#define FILE_NO_EA_KNOWLEDGE 0x00000200
#define FILE_OPEN_FOR_RECOVERY 0x00000400
#define FILE_RANDOM_ACCESS 0x00000800
#define FILE_DELETE_ON_CLOSE 0x00001000
#define FILE_OPEN_BY_FILE_ID 0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT 0x00004000
#define FILE_NO_COMPRESSION 0x00008000
#define FILE_OPEN_REQUIRING_OPLOCK 0x00010000
#define FILE_DISALLOW_EXCLUSIVE 0x00020000
#define FILE_SESSION_AWARE 0x00040000
#define FILE_RESERVE_OPFILTER 0x00100000
#define FILE_OPEN_REPARSE_POINT 0x00200000
#define FILE_OPEN_NO_RECALL 0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY 0x00800000
#endif

/* Reserves and/or commits a region of virtual memory. *BaseAddress and
 * *RegionSize are in/out (0 lets the kernel choose/round); AllocationType is
 * MEM_RESERVE/MEM_COMMIT, Protect the page protection. */
NTSTATUS STDCALL NtAllocateVirtualMemory(
    IN OUT PVOID* BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect);

/* Cancels a pending timer; its signalled state at cancel time is returned in
 * *CurrentState. */
NTSTATUS STDCALL NtCancelTimer(
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL);

/* Sets an event to the non-signalled state. */
NTSTATUS STDCALL NtClearEvent(
    IN HANDLE EventHandle);

/* Closes a handle to any object, releasing the reference it held. */
NTSTATUS STDCALL NtClose(
    IN HANDLE Handle);

/* Creates a namespace directory object and returns a handle to it. */
NTSTATUS STDCALL NtCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes);

/* Creates a named event of the given type with an initial signalled state. */
NTSTATUS STDCALL NtCreateEvent(
    OUT PHANDLE EventHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState);

/* Creates or opens a file/device, returning a handle. CreateDisposition selects
 * create/open/overwrite; CreateOptions carries FILE_* flags. Result detail is in
 * IoStatusBlock. */
NTSTATUS STDCALL NtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions);

/* Creates an I/O completion port with an optional concurrency Count. */
NTSTATUS STDCALL NtCreateIoCompletion(
    OUT PHANDLE IoCompletionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG Count OPTIONAL);

/* Creates a mutant (mutex); InitialOwner grants ownership to the caller. */
NTSTATUS STDCALL NtCreateMutant(
    OUT PHANDLE MutantHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN BOOLEAN InitialOwner);

/* Creates a counting semaphore with an initial and maximum count. */
NTSTATUS STDCALL NtCreateSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount);

/* Creates a timer of the given one-shot/periodic type. */
NTSTATUS STDCALL NtCreateTimer(
    OUT PHANDLE TimerHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN TIMER_TYPE TimerType);

/* Deletes the file named by ObjectAttributes. Returns TRUE on success. */
BOOLEAN STDCALL NtDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes);

/* Issues a device I/O control (IoControlCode) to an open device. Completion is
 * reported via IoStatusBlock and, if given, Event/ApcRoutine. */
NTSTATUS STDCALL NtDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength);

/* Duplicates SourceHandle into TargetHandle; Options may request closing the
 * source or inheriting access. */
NTSTATUS STDCALL NtDuplicateObject(
    IN HANDLE SourceHandle,
    OUT PHANDLE TargetHandle,
    IN ULONG Options);

/* Flushes buffered writes for a file to its device. */
NTSTATUS STDCALL NtFlushBuffersFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock);

/* Decommits and/or releases a virtual memory region reserved by
 * NtAllocateVirtualMemory. FreeType is MEM_DECOMMIT or MEM_RELEASE. */
NTSTATUS STDCALL NtFreeVirtualMemory(
    IN OUT PVOID* BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType);

/* Issues a file-system control (FsControlCode) to an open file/volume. */
NTSTATUS STDCALL NtFsControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength);

/* Opens an existing namespace directory object. */
NTSTATUS STDCALL NtOpenDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes);

/* Opens an existing file/device (never creates). OpenOptions carries FILE_*
 * flags. */
NTSTATUS STDCALL NtOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions);

/* Opens an existing symbolic-link object for querying its target. */
NTSTATUS STDCALL NtOpenSymbolicLinkObject(
    OUT PHANDLE LinkHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes);

/* Changes the page protection of a virtual region to NewProtect; the prior
 * protection is returned in *OldProtect. */
NTSTATUS STDCALL NtProtectVirtualMemory(
    IN OUT PVOID* BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG NewProtect,
    OUT PULONG OldProtect);

/* Pulses an event (signal-then-reset), releasing current waiters; prior state
 * returned in *PreviousState. */
NTSTATUS STDCALL NtPulseEvent(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL);

/* Enumerates directory entries into FileInformation. RestartScan begins at the
 * first entry; FileName filters by pattern. Call repeatedly to page through. */
NTSTATUS STDCALL NtQueryDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN POBJECT_STRING FileName OPTIONAL,
    IN BOOLEAN RestartScan);

/* Enumerates entries of a namespace directory object; *Context tracks the
 * resume position across calls. */
NTSTATUS STDCALL NtQueryDirectoryObject(
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL);

/* Retrieves basic state (type, signalled) for an event. */
NTSTATUS STDCALL NtQueryEvent(
    IN HANDLE EventHandle,
    OUT PEVENT_BASIC_INFORMATION EventInformation);

/* Retrieves size/timestamp/attribute info for a file by name without opening a
 * handle. */
NTSTATUS STDCALL NtQueryFullAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation);

/* Queries FileInformationClass metadata for an open file into FileInformation. */
NTSTATUS STDCALL NtQueryInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass);

/* Retrieves the pending-count state of an I/O completion port. */
NTSTATUS STDCALL NtQueryIoCompletion(
    IN HANDLE IoCompletionHandle,
    OUT PIO_COMPLETION_BASIC_INFORMATION IoCompletionInformation);

/* Retrieves the current count/ownership state of a mutant. */
NTSTATUS STDCALL NtQueryMutant(
    IN HANDLE MutantHandle,
    OUT PMUTANT_BASIC_INFORMATION MutantInformation);

/* Retrieves the current/maximum count of a semaphore. */
NTSTATUS STDCALL NtQuerySemaphore(
    IN HANDLE SemaphoreHandle,
    OUT PSEMAPHORE_BASIC_INFORMATION SemaphoreInformation);

/* Reads the target string of a symbolic-link object into LinkTarget. */
NTSTATUS STDCALL NtQuerySymbolicLinkObject(
    IN HANDLE LinkHandle,
    IN OUT POBJECT_STRING LinkTarget,
    OUT PULONG ReturnedLength OPTIONAL);

/* Retrieves a timer's remaining-time and signalled state. */
NTSTATUS STDCALL NtQueryTimer(
    IN HANDLE TimerHandle,
    OUT PTIMER_BASIC_INFORMATION TimerInformation);

/* Describes the memory region containing BaseAddress (base, size, state,
 * protection). */
NTSTATUS STDCALL NtQueryVirtualMemory(
    IN PVOID BaseAddress,
    OUT PMEMORY_BASIC_INFORMATION MemoryInformation);

/* Queries FsInformationClass metadata for the file's volume into FsInformation. */
NTSTATUS STDCALL NtQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass);

/* Queues a user-mode APC to run ApcRoutine(args) in the target thread. */
NTSTATUS STDCALL NtQueueApcThread(
    IN HANDLE ThreadHandle,
    IN PPS_APC_ROUTINE ApcRoutine,
    IN PVOID ApcArgument1,
    IN PVOID ApcArgument2,
    IN PVOID ApcArgument3);

/* Reads Length bytes from a file into Buffer. ByteOffset gives the position (or
 * uses the file pointer if omitted). Completion via IoStatusBlock/Event/Apc. */
NTSTATUS STDCALL NtReadFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL);

/* Scatter read: reads Length bytes into the page-aligned segments listed in
 * SegmentArray. */
NTSTATUS STDCALL NtReadFileScatter(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL);

/* Releases a mutant owned by the caller; prior count in *PreviousCount. */
NTSTATUS STDCALL NtReleaseMutant(
    IN HANDLE MutantHandle,
    OUT PLONG PreviousCount OPTIONAL);

/* Adds ReleaseCount to a semaphore's count; prior count in *PreviousCount. */
NTSTATUS STDCALL NtReleaseSemaphore(
    IN HANDLE SemaphoreHandle,
    IN LONG ReleaseCount,
    OUT PLONG PreviousCount OPTIONAL);

/* Dequeues a completion packet from an I/O completion port, blocking up to
 * Timeout; the key/apc contexts and status are returned. */
NTSTATUS STDCALL NtRemoveIoCompletion(
    IN HANDLE IoCompletionHandle,
    OUT PVOID* KeyContext,
    OUT PVOID* ApcContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER Timeout);

/* Resumes a suspended thread; prior suspend count in *PreviousSuspendCount. */
NTSTATUS STDCALL NtResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL);

/* Sets an event to signalled; prior state in *PreviousState. */
NTSTATUS STDCALL NtSetEvent(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL);

/* Sets FileInformationClass metadata on an open file (rename, size, position,
 * disposition, etc.). */
NTSTATUS STDCALL NtSetInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass);

/* Posts a completion packet (key/apc contexts, status) to an I/O completion
 * port. */
NTSTATUS STDCALL NtSetIoCompletion(
    IN HANDLE IoCompletionHandle,
    IN PVOID KeyContext,
    IN PVOID ApcContext,
    IN NTSTATUS IoStatus,
    IN ULONG_PTR IoStatusInformation);

/* Sets the system wall-clock time; the prior time is returned in *PreviousTime. */
NTSTATUS STDCALL NtSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER PreviousTime OPTIONAL);

/* Arms a timer to expire at DueTime, optionally periodic (Period ms) and/or
 * firing a TimerApcRoutine; prior state returned in *PreviousState. */
NTSTATUS STDCALL NtSetTimerEx(
    IN HANDLE TimerHandle,
    IN PLARGE_INTEGER DueTime,
    IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
    IN KPROCESSOR_MODE ApcMode,
    IN PVOID TimerContext OPTIONAL,
    IN BOOLEAN ResumeTimer,
    IN LONG Period OPTIONAL,
    OUT PBOOLEAN PreviousState OPTIONAL);

/* Atomically signals SignalHandle and waits on WaitHandle, up to Timeout. */
NTSTATUS STDCALL NtSignalAndWaitForSingleObjectEx(
    IN HANDLE SignalHandle,
    IN HANDLE WaitHandle,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL);

/* Suspends a thread; prior suspend count in *PreviousSuspendCount. */
NTSTATUS STDCALL NtSuspendThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL);

/* Internal APC dispatcher trampoline that delivers I/O completion APCs to user
 * routines. Not called directly by titles. */
VOID STDCALL NtUserIoApcDispatcher(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved);

/* Waits on up to Count handles for all or any (WaitType) to signal, up to
 * Timeout. */
NTSTATUS STDCALL NtWaitForMultipleObjectsEx(
    IN ULONG Count,
    IN CONST HANDLE Handles[],
    IN WAIT_TYPE WaitType,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL);

/* Waits on a single handle until signalled or Timeout elapses. */
NTSTATUS STDCALL NtWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL);

/* As NtWaitForSingleObject, with an explicit wait processor mode. */
NTSTATUS STDCALL NtWaitForSingleObjectEx(
    IN HANDLE Handle,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL);

/* Writes Length bytes from Buffer to a file at ByteOffset (or the file
 * pointer). Completion via IoStatusBlock/Event/Apc. */
NTSTATUS STDCALL NtWriteFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL);

/* Gather write: writes Length bytes from the page-aligned segments in
 * SegmentArray. Returns TRUE on success. */
BOOLEAN STDCALL NtWriteFileGather(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL);

/* Yields the remainder of the current thread's quantum to another ready thread. */
NTSTATUS STDCALL NtYieldExecution(void);

#endif
