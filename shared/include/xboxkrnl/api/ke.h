/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Ke* kernel imports: the core kernel and scheduler. The dispatcher objects
 * (events, semaphores, mutants, timers, queues) with their initialize/set/wait
 * primitives, DPC and APC queuing, thread priority and suspend/resume control,
 * IRQL management, interrupt connection, the wall/interrupt/tick clocks, and the
 * fatal-error bug-check entry points. Many of these have IRQL restrictions --
 * waits require PASSIVE_LEVEL, DPC-related calls run at DISPATCH_LEVEL.
 */

#ifndef XBOXKRNL_API_KE_H
#define XBOXKRNL_API_KE_H

/* Resumes Thread and delivers a pending alert; returns its previous suspend
 * count. */
ULONG STDCALL KeAlertResumeThread(
    IN PKTHREAD Thread);

/* Alerts Thread for the given processor mode, waking it from an alertable wait.
 * Returns TRUE if an alert was already pending. */
BOOLEAN STDCALL KeAlertThread(
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE ProcessorMode);

/* Temporarily raises Thread's priority by Increment (a scheduling boost). */
VOID STDCALL KeBoostPriorityThread(
    IN PKTHREAD Thread,
    IN KPRIORITY Increment);

/* Crashes the console with BugCheckCode. Does not return. */
VOID STDCALL DECLSPEC_NORETURN KeBugCheck(
    IN ULONG BugCheckCode);

/* Crashes the console with a bug-check code and four diagnostic parameters.
 * Does not return. */
VOID STDCALL DECLSPEC_NORETURN KeBugCheckEx(
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4);

/* Cancels a pending timer. Returns TRUE if it was still queued. */
BOOLEAN STDCALL KeCancelTimer(
    IN PKTIMER Timer);

/* Connects an initialized interrupt object to its vector so its service routine
 * begins firing. Returns TRUE on success. */
BOOLEAN STDCALL KeConnectInterrupt(
    IN PKINTERRUPT Interrupt);

/* Blocks the current thread for Interval (100 ns units; negative = relative).
 * Alertable allows alerts to break the wait. */
NTSTATUS STDCALL KeDelayExecutionThread(
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Interval);

/* Disconnects an interrupt object from its vector. Returns TRUE if it was
 * connected. */
BOOLEAN STDCALL KeDisconnectInterrupt(
    IN PKINTERRUPT Interrupt);

/* Enters a critical region, deferring normal-kernel APCs to the current thread.
 * Pair with KeLeaveCriticalRegion. */
VOID STDCALL KeEnterCriticalRegion(void);

/* Returns the current IRQL. */
KIRQL STDCALL KeGetCurrentIrql(void);

/* Returns the currently executing thread object. */
PKTHREAD STDCALL KeGetCurrentThread(void);

/* Initializes an APC object targeting Thread, wiring up its kernel/rundown/
 * normal routines and processor mode for later KeInsertQueueApc. */
VOID STDCALL KeInitializeApc(
    IN PRKAPC Apc,
    IN PRKTHREAD Thread,
    IN PKKERNEL_ROUTINE KernelRoutine,
    IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE NormalRoutine OPTIONAL,
    IN KPROCESSOR_MODE ProcessorMode OPTIONAL,
    IN PVOID NormalContext OPTIONAL);

/* Initializes a device queue to the empty, not-busy state. */
VOID STDCALL KeInitializeDeviceQueue(
    OUT PKDEVICE_QUEUE DeviceQueue);

/* Initializes a DPC object bound to DeferredRoutine(DeferredContext), for later
 * KeInsertQueueDpc. */
VOID STDCALL KeInitializeDpc(
    OUT KDPC* Dpc,
    IN PKDEFERRED_ROUTINE DeferredRoutine,
    IN PVOID DeferredContext OPTIONAL);

/* Initializes an event as notification or synchronization type, with initial
 * signalled State. */
VOID STDCALL KeInitializeEvent(
    IN PRKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State);

/* Initializes an interrupt object binding ServiceRoutine(ServiceContext) to a
 * Vector/Irql, with the given latch mode and vector-sharing. */
VOID STDCALL KeInitializeInterrupt(
    IN PKINTERRUPT Interrupt,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector);

/* Initializes a mutant (mutex); InitialOwner grants it to the caller. */
VOID STDCALL KeInitializeMutant(
    IN PRKMUTANT Mutant,
    IN BOOLEAN InitialOwner);

/* Initializes a queue object with an optional concurrency Count. */
VOID STDCALL KeInitializeQueue(
    IN PRKQUEUE Queue,
    IN ULONG Count OPTIONAL);

/* Initializes a counting semaphore with initial Count and maximum Limit. */
VOID STDCALL KeInitializeSemaphore(
    IN PRKSEMAPHORE Semaphore,
    IN LONG Count,
    IN LONG Limit);

/* Initializes a timer as one-shot (NotificationTimer) or periodic
 * (SynchronizationTimer) per Type. */
VOID STDCALL KeInitializeTimerEx(
    IN PKTIMER Timer,
    IN TIMER_TYPE Type);

/* Inserts a device-queue entry ordered by SortKey; returns FALSE (and does not
 * insert) if the queue was not busy. */
BOOLEAN STDCALL KeInsertByKeyDeviceQueue(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
    IN ULONG SortKey);

/* Inserts a device-queue entry at the tail; returns FALSE if the queue was not
 * busy. */
BOOLEAN STDCALL KeInsertDeviceQueue(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);

/* Inserts Entry at the head of a queue; returns the prior entry count. */
LONG STDCALL KeInsertHeadQueue(
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry);

/* Inserts Entry at the tail of a queue, waking a waiter; returns the prior
 * entry count. */
LONG STDCALL KeInsertQueue(
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry);

/* Queues a previously initialized APC with two arguments and a priority boost.
 * Returns FALSE if the APC was already queued. */
BOOLEAN STDCALL KeInsertQueueApc(
    IN PRKAPC Apc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN KPRIORITY Increment);

/* Queues a DPC for execution at DISPATCH_LEVEL with two arguments. Returns FALSE
 * if it was already queued. */
BOOLEAN STDCALL KeInsertQueueDpc(
    IN PRKDPC Dpc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);

/* Running count of time (100 ns units) spent servicing interrupts. Volatile;
 * read as a KSYSTEM_TIME. */
XBAPI volatile KSYSTEM_TIME KeInterruptTime;

/* Returns TRUE if the current processor is executing a DPC. */
BOOLEAN STDCALL KeIsExecutingDpc(void);

/* Leaves a critical region entered with KeEnterCriticalRegion, allowing
 * deferred APCs to run. */
VOID STDCALL KeLeaveCriticalRegion(void);

/* Pulses an event: signals it, releases all current waiters, then resets it.
 * Returns the prior signal state. */
LONG STDCALL KePulseEvent(
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait);

/* Returns Thread's base priority. */
LONG STDCALL KeQueryBasePriorityThread(
    IN PKTHREAD Thread);

/* Returns cumulative interrupt time in 100 ns units. */
ULONGLONG STDCALL KeQueryInterruptTime(void);

/* Returns the high-resolution performance counter's current tick. */
ULONGLONG STDCALL KeQueryPerformanceCounter(void);

/* Returns the performance counter's frequency in ticks per second. */
ULONGLONG STDCALL KeQueryPerformanceFrequency(void);

/* Writes the current system (wall-clock) time, in 100 ns units since 1601, to
 * *CurrentTime. */
VOID STDCALL KeQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime);

/* Raises IRQL to DISPATCH_LEVEL and returns the previous IRQL. */
KIRQL STDCALL KeRaiseIrqlToDpcLevel(void);

/* Raises IRQL to SYNCH_LEVEL and returns the previous IRQL. */
KIRQL STDCALL KeRaiseIrqlToSynchLevel(void);

/* Releases a mutant. Abandoned marks it abandoned; Increment boosts a woken
 * waiter. Returns the prior signal state. */
LONG STDCALL KeReleaseMutant(
    IN PRKMUTANT Mutant,
    IN KPRIORITY Increment,
    IN BOOLEAN Abandoned,
    IN BOOLEAN Wait);

/* Adds Adjustment to a semaphore's count, releasing waiters. Returns the prior
 * count. */
LONG STDCALL KeReleaseSemaphore(
    IN PRKSEMAPHORE Semaphore,
    IN KPRIORITY Increment,
    IN LONG Adjustment,
    IN BOOLEAN Wait);

/* Removes the device-queue entry matching SortKey, or NULL if none/queue idle. */
PKDEVICE_QUEUE_ENTRY STDCALL KeRemoveByKeyDeviceQueue(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey);

/* Removes and returns the head device-queue entry; NULL leaves the queue not
 * busy. */
PKDEVICE_QUEUE_ENTRY STDCALL KeRemoveDeviceQueue(
    IN PKDEVICE_QUEUE DeviceQueue);

/* Removes a specific entry from a device queue; returns TRUE if it was present. */
BOOLEAN STDCALL KeRemoveEntryDeviceQueue(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE DeviceQueueEntry);

/* Removes the next entry from a queue, blocking up to Timeout if empty. Returns
 * the entry, or a status-coded pointer on timeout. */
PLIST_ENTRY STDCALL KeRemoveQueue(
    IN PRKQUEUE Queue,
    IN KPROCESSOR_MODE WaitMode,
    IN PLARGE_INTEGER Timeout OPTIONAL);

/* Removes a DPC from its queue before it runs; returns TRUE if it was queued. */
BOOLEAN STDCALL KeRemoveQueueDpc(
    IN PRKDPC Dpc);

/* Resets an event to non-signalled and returns its prior state. */
LONG STDCALL KeResetEvent(
    IN PRKEVENT Event);

/* Restores x87/SSE state previously saved by KeSaveFloatingPointState. */
NTSTATUS STDCALL KeRestoreFloatingPointState(
    IN PKFLOATING_SAVE FloatSave);

/* Decrements Thread's suspend count, resuming it at zero. Returns the prior
 * count. */
ULONG STDCALL KeResumeThread(
    IN PKTHREAD Thread);

/* Tears down a queue, returning its remaining entries as a list. */
PLIST_ENTRY STDCALL KeRundownQueue(
    IN PRKQUEUE Queue);

/* Saves the current x87/SSE state into FloatSave for later restore. */
NTSTATUS STDCALL KeSaveFloatingPointState(
    OUT PKFLOATING_SAVE FloatSave);

/* Sets Thread's base priority relative by Increment; returns the prior base. */
LONG STDCALL KeSetBasePriorityThread(
    IN PKTHREAD Thread,
    IN LONG Increment);

/* Enables or disables scheduling boosts for Thread; returns the prior setting. */
LOGICAL STDCALL KeSetDisableBoostThread(
    IN PKTHREAD Thread,
    IN LOGICAL Disable);

/* Signals an event, waking waiters with a priority Increment. Returns the prior
 * signal state. */
LONG STDCALL KeSetEvent(
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait);

/* Signals an event and boosts the woken thread; the released thread is returned
 * in *Thread when provided. */
VOID STDCALL KeSetEventBoostPriority(
    IN PRKEVENT Event,
    IN PRKTHREAD* Thread OPTIONAL);

/* Sets the process-wide base priority; returns the prior value. */
KPRIORITY STDCALL KeSetPriorityProcess(
    IN PKPROCESS Process,
    IN KPRIORITY BasePriority);

/* Sets Thread's priority; returns the prior value. */
KPRIORITY STDCALL KeSetPriorityThread(
    IN PKTHREAD Thread,
    IN KPRIORITY Priority);

/* Arms a one-shot timer to expire at DueTime (absolute if positive, relative if
 * negative, 100 ns units), optionally queuing Dpc on expiry. */
BOOLEAN STDCALL KeSetTimer(
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN PKDPC Dpc OPTIONAL);

/* As KeSetTimer, but a nonzero Period (ms) makes the timer periodic. */
BOOLEAN STDCALL KeSetTimerEx(
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN LONG Period OPTIONAL,
    IN PKDPC Dpc OPTIONAL);

/* Busy-waits (spins) for MicroSeconds without yielding the processor. */
VOID STDCALL KeStallExecutionProcessor(
    IN ULONG MicroSeconds);

/* Increments Thread's suspend count, suspending it. Returns the prior count. */
ULONG STDCALL KeSuspendThread(
    IN PKTHREAD Thread);

/* Runs SynchronizeRoutine(SynchronizeContext) at the interrupt's IRQL holding
 * its spin lock, so it can safely touch shared ISR state. Returns the routine's
 * result. */
BOOLEAN STDCALL KeSynchronizeExecution(
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext);

/* Current system (wall-clock) time in 100 ns units. Volatile. */
XBAPI volatile KSYSTEM_TIME KeSystemTime;

/* Tests and clears the current thread's alert flag for the given mode; returns
 * whether an alert was pending. */
BOOLEAN STDCALL KeTestAlertThread(
    IN KPROCESSOR_MODE ProcessorMode);

/* Count of clock ticks since boot. Volatile. */
XBAPI volatile DWORD KeTickCount;

/* Length of one clock tick in 100 ns units (the timer resolution). */
XBAPI CONST ULONG KeTimeIncrement;

/* Waits until Count dispatcher objects satisfy WaitType (all or any), or until
 * Timeout. WaitBlockArray supplies the per-object wait blocks. Alertable allows
 * alerts to break the wait. */
NTSTATUS STDCALL KeWaitForMultipleObjects(
    IN ULONG Count,
    IN PVOID Object[],
    IN WAIT_TYPE WaitType,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    IN PKWAIT_BLOCK WaitBlockArray);

/* Waits until a single dispatcher Object is signalled or Timeout elapses.
 * Timeout NULL waits indefinitely; a zero timeout polls. */
NTSTATUS STDCALL KeWaitForSingleObject(
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL);

#endif
