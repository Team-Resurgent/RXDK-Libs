#ifndef XBOXKRNL_API_KE_H
#define XBOXKRNL_API_KE_H

XBAPI ULONG STDCALL KeAlertResumeThread
(
    IN PKTHREAD Thread
);

XBAPI BOOLEAN STDCALL KeAlertThread
(
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE ProcessorMode
);

XBAPI VOID STDCALL KeBoostPriorityThread
(
    IN PKTHREAD Thread,
    IN KPRIORITY Increment
);

XBAPI VOID STDCALL DECLSPEC_NORETURN KeBugCheck
(
    IN ULONG BugCheckCode
);

XBAPI VOID STDCALL DECLSPEC_NORETURN KeBugCheckEx
(
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4
);

XBAPI BOOLEAN STDCALL KeCancelTimer
(
    IN PKTIMER Timer
);

XBAPI BOOLEAN STDCALL KeConnectInterrupt
(
    IN PKINTERRUPT Interrupt
);

XBAPI NTSTATUS STDCALL KeDelayExecutionThread
(
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Interval
);

XBAPI BOOLEAN STDCALL KeDisconnectInterrupt
(
    IN PKINTERRUPT Interrupt
);

XBAPI VOID STDCALL KeEnterCriticalRegion(void);

XBAPI KIRQL STDCALL KeGetCurrentIrql(void);

XBAPI PKTHREAD STDCALL KeGetCurrentThread(void);

XBAPI VOID STDCALL KeInitializeApc
(
    IN PRKAPC Apc,
    IN PRKTHREAD Thread,
    IN PKKERNEL_ROUTINE KernelRoutine,
    IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE NormalRoutine OPTIONAL,
    IN KPROCESSOR_MODE ProcessorMode OPTIONAL,
    IN PVOID NormalContext OPTIONAL
);

XBAPI VOID STDCALL KeInitializeDeviceQueue
(
    OUT PKDEVICE_QUEUE DeviceQueue
);

XBAPI VOID STDCALL KeInitializeDpc
(
    OUT KDPC *Dpc,
    IN PKDEFERRED_ROUTINE DeferredRoutine,
    IN PVOID DeferredContext OPTIONAL
);

XBAPI VOID STDCALL KeInitializeEvent
(
    IN PRKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State
);

XBAPI VOID STDCALL KeInitializeInterrupt
(
    IN PKINTERRUPT Interrupt,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector
);

XBAPI VOID STDCALL KeInitializeMutant
(
    IN PRKMUTANT Mutant,
    IN BOOLEAN InitialOwner
);

XBAPI VOID STDCALL KeInitializeQueue
(
    IN PRKQUEUE Queue,
    IN ULONG Count OPTIONAL
);

XBAPI VOID STDCALL KeInitializeSemaphore
(
    IN PRKSEMAPHORE Semaphore,
    IN LONG Count,
    IN LONG Limit
);

XBAPI VOID STDCALL KeInitializeTimerEx
(
    IN PKTIMER Timer,
    IN TIMER_TYPE Type
);

XBAPI BOOLEAN STDCALL KeInsertByKeyDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
    IN ULONG SortKey
);

XBAPI BOOLEAN STDCALL KeInsertDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
);

XBAPI LONG STDCALL KeInsertHeadQueue
(
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry
);

XBAPI LONG STDCALL KeInsertQueue
(
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry
);

XBAPI BOOLEAN STDCALL KeInsertQueueApc
(
    IN PRKAPC Apc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN KPRIORITY Increment
);

XBAPI BOOLEAN STDCALL KeInsertQueueDpc
(
    IN PRKDPC Dpc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

XBAPI volatile KSYSTEM_TIME KeInterruptTime;

XBAPI BOOLEAN STDCALL KeIsExecutingDpc (void);

XBAPI VOID STDCALL KeLeaveCriticalRegion (void);

XBAPI LONG STDCALL KePulseEvent
(
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
);

XBAPI LONG STDCALL KeQueryBasePriorityThread
(
    IN PKTHREAD Thread
);

XBAPI ULONGLONG STDCALL KeQueryInterruptTime (void);

XBAPI ULONGLONG STDCALL KeQueryPerformanceCounter(void);

XBAPI ULONGLONG STDCALL KeQueryPerformanceFrequency(void);

XBAPI VOID STDCALL KeQuerySystemTime
(
    OUT PLARGE_INTEGER CurrentTime
);

XBAPI KIRQL STDCALL KeRaiseIrqlToDpcLevel (void);

XBAPI KIRQL STDCALL KeRaiseIrqlToSynchLevel (void);

XBAPI LONG STDCALL KeReleaseMutant
(
    IN PRKMUTANT Mutant,
    IN KPRIORITY Increment,
    IN BOOLEAN Abandoned,
    IN BOOLEAN Wait
);

XBAPI LONG STDCALL KeReleaseSemaphore
(
    IN PRKSEMAPHORE Semaphore,
    IN KPRIORITY Increment,
    IN LONG Adjustment,
    IN BOOLEAN Wait
);

XBAPI PKDEVICE_QUEUE_ENTRY STDCALL KeRemoveByKeyDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
);

XBAPI PKDEVICE_QUEUE_ENTRY STDCALL KeRemoveDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue
);

XBAPI BOOLEAN STDCALL KeRemoveEntryDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE DeviceQueueEntry
);

XBAPI PLIST_ENTRY STDCALL KeRemoveQueue
(
    IN PRKQUEUE Queue,
    IN KPROCESSOR_MODE WaitMode,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

XBAPI BOOLEAN STDCALL KeRemoveQueueDpc
(
    IN PRKDPC Dpc
);

XBAPI LONG STDCALL KeResetEvent
(
    IN PRKEVENT Event
);

XBAPI NTSTATUS STDCALL KeRestoreFloatingPointState
(
    IN PKFLOATING_SAVE FloatSave
);

XBAPI ULONG STDCALL KeResumeThread
(
    IN PKTHREAD Thread
);

XBAPI PLIST_ENTRY STDCALL KeRundownQueue
(
    IN PRKQUEUE Queue
);

XBAPI NTSTATUS STDCALL KeSaveFloatingPointState
(
    OUT PKFLOATING_SAVE FloatSave
);

XBAPI LONG STDCALL KeSetBasePriorityThread
(
    IN PKTHREAD Thread,
    IN LONG Increment
);

XBAPI LOGICAL STDCALL KeSetDisableBoostThread
(
    IN PKTHREAD Thread,
    IN LOGICAL Disable
);

XBAPI LONG STDCALL KeSetEvent
(
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
);

XBAPI VOID STDCALL KeSetEventBoostPriority
(
    IN PRKEVENT Event,
    IN PRKTHREAD *Thread OPTIONAL
);

XBAPI KPRIORITY STDCALL KeSetPriorityProcess
(
    IN PKPROCESS Process,
    IN KPRIORITY BasePriority
);

XBAPI KPRIORITY STDCALL KeSetPriorityThread
(
    IN PKTHREAD Thread,
    IN KPRIORITY Priority
);

XBAPI BOOLEAN STDCALL KeSetTimer
(
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN PKDPC Dpc OPTIONAL
);

XBAPI BOOLEAN STDCALL KeSetTimerEx
(
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN LONG Period OPTIONAL,
    IN PKDPC Dpc OPTIONAL
);

XBAPI VOID STDCALL KeStallExecutionProcessor
(
    IN ULONG MicroSeconds
);

XBAPI ULONG STDCALL KeSuspendThread
(
    IN PKTHREAD Thread
);

XBAPI BOOLEAN STDCALL KeSynchronizeExecution
(
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
);

XBAPI volatile KSYSTEM_TIME KeSystemTime;

XBAPI BOOLEAN STDCALL KeTestAlertThread
(
    IN KPROCESSOR_MODE ProcessorMode
);

XBAPI volatile DWORD KeTickCount;

XBAPI CONST ULONG KeTimeIncrement;

XBAPI NTSTATUS STDCALL KeWaitForMultipleObjects
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

XBAPI NTSTATUS STDCALL KeWaitForSingleObject
(
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

#endif
