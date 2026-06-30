#ifndef XBOXKRNL_API_KE_H
#define XBOXKRNL_API_KE_H

ULONG STDCALL KeAlertResumeThread
(
    IN PKTHREAD Thread
);

BOOLEAN STDCALL KeAlertThread
(
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE ProcessorMode
);

VOID STDCALL KeBoostPriorityThread
(
    IN PKTHREAD Thread,
    IN KPRIORITY Increment
);

VOID STDCALL DECLSPEC_NORETURN KeBugCheck
(
    IN ULONG BugCheckCode
);

VOID STDCALL DECLSPEC_NORETURN KeBugCheckEx
(
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4
);

BOOLEAN STDCALL KeCancelTimer
(
    IN PKTIMER Timer
);

BOOLEAN STDCALL KeConnectInterrupt
(
    IN PKINTERRUPT Interrupt
);

NTSTATUS STDCALL KeDelayExecutionThread
(
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Interval
);

BOOLEAN STDCALL KeDisconnectInterrupt
(
    IN PKINTERRUPT Interrupt
);

VOID STDCALL KeEnterCriticalRegion(void);

KIRQL STDCALL KeGetCurrentIrql(void);

PKTHREAD STDCALL KeGetCurrentThread(void);

VOID STDCALL KeInitializeApc
(
    IN PRKAPC Apc,
    IN PRKTHREAD Thread,
    IN PKKERNEL_ROUTINE KernelRoutine,
    IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE NormalRoutine OPTIONAL,
    IN KPROCESSOR_MODE ProcessorMode OPTIONAL,
    IN PVOID NormalContext OPTIONAL
);

VOID STDCALL KeInitializeDeviceQueue
(
    OUT PKDEVICE_QUEUE DeviceQueue
);

VOID STDCALL KeInitializeDpc
(
    OUT KDPC *Dpc,
    IN PKDEFERRED_ROUTINE DeferredRoutine,
    IN PVOID DeferredContext OPTIONAL
);

VOID STDCALL KeInitializeEvent
(
    IN PRKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State
);

VOID STDCALL KeInitializeInterrupt
(
    IN PKINTERRUPT Interrupt,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector
);

VOID STDCALL KeInitializeMutant
(
    IN PRKMUTANT Mutant,
    IN BOOLEAN InitialOwner
);

VOID STDCALL KeInitializeQueue
(
    IN PRKQUEUE Queue,
    IN ULONG Count OPTIONAL
);

VOID STDCALL KeInitializeSemaphore
(
    IN PRKSEMAPHORE Semaphore,
    IN LONG Count,
    IN LONG Limit
);

VOID STDCALL KeInitializeTimerEx
(
    IN PKTIMER Timer,
    IN TIMER_TYPE Type
);

BOOLEAN STDCALL KeInsertByKeyDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
    IN ULONG SortKey
);

BOOLEAN STDCALL KeInsertDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
);

LONG STDCALL KeInsertHeadQueue
(
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry
);

LONG STDCALL KeInsertQueue
(
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry
);

BOOLEAN STDCALL KeInsertQueueApc
(
    IN PRKAPC Apc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN KPRIORITY Increment
);

BOOLEAN STDCALL KeInsertQueueDpc
(
    IN PRKDPC Dpc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

XBAPI volatile KSYSTEM_TIME KeInterruptTime;

BOOLEAN STDCALL KeIsExecutingDpc (void);

VOID STDCALL KeLeaveCriticalRegion (void);

LONG STDCALL KePulseEvent
(
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
);

LONG STDCALL KeQueryBasePriorityThread
(
    IN PKTHREAD Thread
);

ULONGLONG STDCALL KeQueryInterruptTime (void);

ULONGLONG STDCALL KeQueryPerformanceCounter(void);

ULONGLONG STDCALL KeQueryPerformanceFrequency(void);

VOID STDCALL KeQuerySystemTime
(
    OUT PLARGE_INTEGER CurrentTime
);

KIRQL STDCALL KeRaiseIrqlToDpcLevel (void);

KIRQL STDCALL KeRaiseIrqlToSynchLevel (void);

LONG STDCALL KeReleaseMutant
(
    IN PRKMUTANT Mutant,
    IN KPRIORITY Increment,
    IN BOOLEAN Abandoned,
    IN BOOLEAN Wait
);

LONG STDCALL KeReleaseSemaphore
(
    IN PRKSEMAPHORE Semaphore,
    IN KPRIORITY Increment,
    IN LONG Adjustment,
    IN BOOLEAN Wait
);

PKDEVICE_QUEUE_ENTRY STDCALL KeRemoveByKeyDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
);

PKDEVICE_QUEUE_ENTRY STDCALL KeRemoveDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue
);

BOOLEAN STDCALL KeRemoveEntryDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE DeviceQueueEntry
);

PLIST_ENTRY STDCALL KeRemoveQueue
(
    IN PRKQUEUE Queue,
    IN KPROCESSOR_MODE WaitMode,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

BOOLEAN STDCALL KeRemoveQueueDpc
(
    IN PRKDPC Dpc
);

LONG STDCALL KeResetEvent
(
    IN PRKEVENT Event
);

NTSTATUS STDCALL KeRestoreFloatingPointState
(
    IN PKFLOATING_SAVE FloatSave
);

ULONG STDCALL KeResumeThread
(
    IN PKTHREAD Thread
);

PLIST_ENTRY STDCALL KeRundownQueue
(
    IN PRKQUEUE Queue
);

NTSTATUS STDCALL KeSaveFloatingPointState
(
    OUT PKFLOATING_SAVE FloatSave
);

LONG STDCALL KeSetBasePriorityThread
(
    IN PKTHREAD Thread,
    IN LONG Increment
);

LOGICAL STDCALL KeSetDisableBoostThread
(
    IN PKTHREAD Thread,
    IN LOGICAL Disable
);

LONG STDCALL KeSetEvent
(
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
);

VOID STDCALL KeSetEventBoostPriority
(
    IN PRKEVENT Event,
    IN PRKTHREAD *Thread OPTIONAL
);

KPRIORITY STDCALL KeSetPriorityProcess
(
    IN PKPROCESS Process,
    IN KPRIORITY BasePriority
);

KPRIORITY STDCALL KeSetPriorityThread
(
    IN PKTHREAD Thread,
    IN KPRIORITY Priority
);

BOOLEAN STDCALL KeSetTimer
(
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN PKDPC Dpc OPTIONAL
);

BOOLEAN STDCALL KeSetTimerEx
(
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN LONG Period OPTIONAL,
    IN PKDPC Dpc OPTIONAL
);

VOID STDCALL KeStallExecutionProcessor
(
    IN ULONG MicroSeconds
);

ULONG STDCALL KeSuspendThread
(
    IN PKTHREAD Thread
);

BOOLEAN STDCALL KeSynchronizeExecution
(
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
);

XBAPI volatile KSYSTEM_TIME KeSystemTime;

BOOLEAN STDCALL KeTestAlertThread
(
    IN KPROCESSOR_MODE ProcessorMode
);

XBAPI volatile DWORD KeTickCount;

XBAPI CONST ULONG KeTimeIncrement;

NTSTATUS STDCALL KeWaitForMultipleObjects
(
    IN ULONG Count,
    IN PVOID Object[],
    IN WAIT_TYPE WaitType,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    IN PKWAIT_BLOCK WaitBlockArray
);

NTSTATUS STDCALL KeWaitForSingleObject
(
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

#endif
