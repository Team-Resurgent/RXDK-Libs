#ifndef XBOXKRNL_API_KE_H
#define XBOXKRNL_API_KE_H

XBAPI ULONG NTAPI KeAlertResumeThread
(
    IN PKTHREAD Thread
);

XBAPI BOOLEAN NTAPI KeAlertThread
(
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE ProcessorMode
);

XBAPI VOID NTAPI KeBoostPriorityThread
(
    IN PKTHREAD Thread,
    IN KPRIORITY Increment
);

XBAPI VOID NTAPI DECLSPEC_NORETURN KeBugCheck
(
    IN ULONG BugCheckCode
);

XBAPI VOID NTAPI DECLSPEC_NORETURN KeBugCheckEx
(
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4
);

XBAPI BOOLEAN NTAPI KeCancelTimer
(
    IN PKTIMER Timer
);

XBAPI BOOLEAN NTAPI KeConnectInterrupt
(
    IN PKINTERRUPT Interrupt
);

XBAPI NTSTATUS NTAPI KeDelayExecutionThread
(
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Interval
);

XBAPI BOOLEAN NTAPI KeDisconnectInterrupt
(
    IN PKINTERRUPT Interrupt
);

XBAPI VOID NTAPI KeEnterCriticalRegion(void);

XBAPI KIRQL NTAPI KeGetCurrentIrql(void);

XBAPI PKTHREAD NTAPI KeGetCurrentThread(void);

XBAPI VOID NTAPI KeInitializeApc
(
    IN PRKAPC Apc,
    IN PRKTHREAD Thread,
    IN PKKERNEL_ROUTINE KernelRoutine,
    IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE NormalRoutine OPTIONAL,
    IN KPROCESSOR_MODE ProcessorMode OPTIONAL,
    IN PVOID NormalContext OPTIONAL
);

XBAPI VOID NTAPI KeInitializeDeviceQueue
(
    OUT PKDEVICE_QUEUE DeviceQueue
);

XBAPI VOID NTAPI KeInitializeDpc
(
    OUT KDPC *Dpc,
    IN PKDEFERRED_ROUTINE DeferredRoutine,
    IN PVOID DeferredContext OPTIONAL
);

XBAPI VOID NTAPI KeInitializeEvent
(
    IN PRKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State
);

XBAPI VOID NTAPI KeInitializeInterrupt
(
    IN PKINTERRUPT Interrupt,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector
);

XBAPI VOID NTAPI KeInitializeMutant
(
    IN PRKMUTANT Mutant,
    IN BOOLEAN InitialOwner
);

XBAPI VOID NTAPI KeInitializeQueue
(
    IN PRKQUEUE Queue,
    IN ULONG Count OPTIONAL
);

XBAPI VOID NTAPI KeInitializeSemaphore
(
    IN PRKSEMAPHORE Semaphore,
    IN LONG Count,
    IN LONG Limit
);

XBAPI VOID NTAPI KeInitializeTimerEx
(
    IN PKTIMER Timer,
    IN TIMER_TYPE Type
);

XBAPI BOOLEAN NTAPI KeInsertByKeyDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
    IN ULONG SortKey
);

XBAPI BOOLEAN NTAPI KeInsertDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
);

XBAPI LONG NTAPI KeInsertHeadQueue
(
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry
);

XBAPI LONG NTAPI KeInsertQueue
(
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry
);

XBAPI BOOLEAN NTAPI KeInsertQueueApc
(
    IN PRKAPC Apc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN KPRIORITY Increment
);

XBAPI BOOLEAN NTAPI KeInsertQueueDpc
(
    IN PRKDPC Dpc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

XBAPI volatile KSYSTEM_TIME KeInterruptTime;

XBAPI BOOLEAN NTAPI KeIsExecutingDpc (void);

XBAPI VOID NTAPI KeLeaveCriticalRegion (void);

XBAPI LONG NTAPI KePulseEvent
(
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
);

XBAPI LONG NTAPI KeQueryBasePriorityThread
(
    IN PKTHREAD Thread
);

XBAPI ULONGLONG NTAPI KeQueryInterruptTime (void);

XBAPI ULONGLONG NTAPI KeQueryPerformanceCounter(void);

XBAPI ULONGLONG NTAPI KeQueryPerformanceFrequency(void);

XBAPI VOID NTAPI KeQuerySystemTime
(
    OUT PLARGE_INTEGER CurrentTime
);

XBAPI KIRQL NTAPI KeRaiseIrqlToDpcLevel (void);

XBAPI KIRQL NTAPI KeRaiseIrqlToSynchLevel (void);

XBAPI LONG NTAPI KeReleaseMutant
(
    IN PRKMUTANT Mutant,
    IN KPRIORITY Increment,
    IN BOOLEAN Abandoned,
    IN BOOLEAN Wait
);

XBAPI LONG NTAPI KeReleaseSemaphore
(
    IN PRKSEMAPHORE Semaphore,
    IN KPRIORITY Increment,
    IN LONG Adjustment,
    IN BOOLEAN Wait
);

XBAPI PKDEVICE_QUEUE_ENTRY NTAPI KeRemoveByKeyDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
);

XBAPI PKDEVICE_QUEUE_ENTRY NTAPI KeRemoveDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue
);

XBAPI BOOLEAN NTAPI KeRemoveEntryDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE DeviceQueueEntry
);

XBAPI PLIST_ENTRY NTAPI KeRemoveQueue
(
    IN PRKQUEUE Queue,
    IN KPROCESSOR_MODE WaitMode,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

XBAPI BOOLEAN NTAPI KeRemoveQueueDpc
(
    IN PRKDPC Dpc
);

XBAPI LONG NTAPI KeResetEvent
(
    IN PRKEVENT Event
);

XBAPI NTSTATUS NTAPI KeRestoreFloatingPointState
(
    IN PKFLOATING_SAVE FloatSave
);

XBAPI ULONG NTAPI KeResumeThread
(
    IN PKTHREAD Thread
);

XBAPI PLIST_ENTRY NTAPI KeRundownQueue
(
    IN PRKQUEUE Queue
);

XBAPI NTSTATUS NTAPI KeSaveFloatingPointState
(
    OUT PKFLOATING_SAVE FloatSave
);

XBAPI LONG NTAPI KeSetBasePriorityThread
(
    IN PKTHREAD Thread,
    IN LONG Increment
);

XBAPI LOGICAL NTAPI KeSetDisableBoostThread
(
    IN PKTHREAD Thread,
    IN LOGICAL Disable
);

XBAPI LONG NTAPI KeSetEvent
(
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
);

XBAPI VOID NTAPI KeSetEventBoostPriority
(
    IN PRKEVENT Event,
    IN PRKTHREAD *Thread OPTIONAL
);

XBAPI KPRIORITY NTAPI KeSetPriorityProcess
(
    IN PKPROCESS Process,
    IN KPRIORITY BasePriority
);

XBAPI KPRIORITY NTAPI KeSetPriorityThread
(
    IN PKTHREAD Thread,
    IN KPRIORITY Priority
);

XBAPI BOOLEAN NTAPI KeSetTimer
(
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN PKDPC Dpc OPTIONAL
);

XBAPI BOOLEAN NTAPI KeSetTimerEx
(
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN LONG Period OPTIONAL,
    IN PKDPC Dpc OPTIONAL
);

XBAPI VOID NTAPI KeStallExecutionProcessor
(
    IN ULONG MicroSeconds
);

XBAPI ULONG NTAPI KeSuspendThread
(
    IN PKTHREAD Thread
);

XBAPI BOOLEAN NTAPI KeSynchronizeExecution
(
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
);

XBAPI volatile KSYSTEM_TIME KeSystemTime;

XBAPI BOOLEAN NTAPI KeTestAlertThread
(
    IN KPROCESSOR_MODE ProcessorMode
);

XBAPI volatile DWORD KeTickCount;

XBAPI CONST ULONG KeTimeIncrement;

XBAPI NTSTATUS NTAPI KeWaitForMultipleObjects
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

XBAPI NTSTATUS NTAPI KeWaitForSingleObject
(
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

#endif
