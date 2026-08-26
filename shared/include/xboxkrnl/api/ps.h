/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Ps* kernel imports: thread creation and process bookkeeping. The Xbox has a
 * single process, so these deal in threads: the simple and extended thread
 * creators, the create-thread notification hook, statistics, and the thread
 * object type. CreateThread in libxapi is layered on top of these.
 */

#ifndef XBOXKRNL_API_PS_H
#define XBOXKRNL_API_PS_H

/* Creates a system thread running StartRoutine(StartContext). The thread handle
 * is returned in *ThreadHandle and, if provided, its id in *ThreadId.
 * DebuggerThread marks it as owned by the debugger. Returns an NTSTATUS. */
NTSTATUS STDCALL PsCreateSystemThread(
    OUT PHANDLE ThreadHandle,
    OUT PHANDLE ThreadId OPTIONAL,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN BOOLEAN DebuggerThread);

/* Extended thread creation with explicit sizing and startup control:
 * ThreadExtensionSize/KernelStackSize/TlsDataSize size the thread's storage,
 * CreateSuspended leaves it not yet scheduled, and SystemRoutine (if given) runs
 * before StartRoutine. Returns an NTSTATUS. */
NTSTATUS STDCALL PsCreateSystemThreadEx(
    OUT PHANDLE ThreadHandle,
    IN SIZE_T ThreadExtensionSize,
    IN SIZE_T KernelStackSize,
    IN SIZE_T TlsDataSize,
    OUT PHANDLE ThreadId OPTIONAL,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN BOOLEAN CreateSuspended,
    IN BOOLEAN DebuggerThread,
    IN PKSYSTEM_ROUTINE SystemRoutine OPTIONAL);

/* Fills ProcessStatistics with process/thread accounting figures. */
NTSTATUS STDCALL PsQueryStatistics(
    IN OUT PPS_STATISTICS ProcessStatistics);

/* Registers (or, with NULL, removes) a callback invoked whenever a thread is
 * created or destroyed. Returns an NTSTATUS. */
NTSTATUS STDCALL PsSetCreateThreadNotifyRoutine(
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

/* The kernel object type for threads; passed to the Ob* routines. */
XBAPI OBJECT_TYPE PsThreadObjectType[1];

#endif
