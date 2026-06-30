#ifndef XBOXKRNL_TYPES_KERNEL_H
#define XBOXKRNL_TYPES_KERNEL_H

typedef enum _EVENT_TYPE {
    NotificationEvent = 0,
    SynchronizationEvent
} EVENT_TYPE;

typedef struct _MM_STATISTICS {
    ULONG Length;
    ULONG TotalPhysicalPages;
    ULONG AvailablePages;
    ULONG VirtualMemoryBytesCommitted;
    ULONG VirtualMemoryBytesReserved;
    ULONG CachePagesCommitted;
    ULONG PoolPagesCommitted;
    ULONG StackPagesCommitted;
    ULONG ImagePagesCommitted;
} MM_STATISTICS, *PMM_STATISTICS;

typedef struct _LAUNCH_DATA_HEADER {
    DWORD dwLaunchDataType;
    DWORD dwTitleId;
    CHAR szLaunchPath[520];
    DWORD dwFlags;
} LAUNCH_DATA_HEADER, *PLAUNCH_DATA_HEADER;

typedef struct _LAUNCH_DATA_PAGE {
    LAUNCH_DATA_HEADER Header;
    UCHAR Pad[492];
    UCHAR LaunchData[3072];
} LAUNCH_DATA_PAGE, *PLAUNCH_DATA_PAGE;

#define LDT_TITLE                                             0
#define LDT_LAUNCH_DASHBOARD                                  1
#define LDT_FROM_DASHBOARD                                    2
#define LDT_NONE                                              0xFFFFFFFF
#define LDT_LAUNCH_DASHBOARD_REASON_NONE                      0
#define LDT_LAUNCH_DASHBOARD_REASON_ERROR                     1
#define LDT_LAUNCH_DASHBOARD_REASON_SAVEDATA                  2
#define LDT_LAUNCH_DASHBOARD_REASON_SETTINGS                  3
#define LDT_LAUNCH_DASHBOARD_REASON_MUSIC                     4
#define LDT_LAUNCH_DASHBOARD_REASON_NETWORK                   6
#define LDT_LAUNCH_DASHBOARD_REASON_NEW_ACCOUNT               7
#define LDT_LAUNCH_DASHBOARD_REASON_SERVER_INFO               8
#define LDT_LAUNCH_DASHBOARD_REASON_SHOW_POLICIES             9
#define LDT_LAUNCH_DASHBOARD_REASON_ONLINE_MENU               10
#define LDT_LAUNCH_DASHBOARD_REASON_FORCE_ACCOUNT_NAME_CHANGE 11
#define LDT_LAUNCH_DASHBOARD_REASON_FORCE_BILLING_CHANGE      12

typedef struct _DISPATCHER_HEADER {
    UCHAR Type;
    UCHAR Absolute;
    UCHAR Size;
    UCHAR Inserted;
    LONG SignalState;
    LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER;

typedef struct _KDPC {
    CSHORT Type;
    BOOLEAN Inserted;
    UCHAR Padding;
    LIST_ENTRY DpcListEntry;
    PVOID DeferredRoutine;
    PVOID DeferredContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
} KDPC, *PKDPC, *RESTRICTED_POINTER PRKDPC;

typedef struct _KTIMER {
    DISPATCHER_HEADER Header;
    ULARGE_INTEGER DueTime;
    LIST_ENTRY TimerListEntry;
    PKDPC Dpc;
    LONG Period;
} KTIMER, *PKTIMER;

typedef struct _KPROCESS {
    LIST_ENTRY ReadListHead;
    LIST_ENTRY ThreadListHead;
    ULONG StackCount;
    LONG ThreadQuantum;
    SCHAR BasePriority;
    UCHAR DisableBoost;
    UCHAR DisableQuantum;
} KPROCESS, *PKPROCESS;

typedef struct _KAPC_STATE {
    LIST_ENTRY ApcListHead[2];
    PKPROCESS Process;
    UCHAR KernelApcInProgress;
    UCHAR KernelApcPending;
    UCHAR UserApcPending;
    UCHAR ApcQueueable;
} KAPC_STATE;

typedef struct _KQUEUE {
    DISPATCHER_HEADER Header;
    LIST_ENTRY EntryListHead;
    ULONG CurrentCount;
    ULONG MaximumCount;
    LIST_ENTRY ThreadListHead;
} KQUEUE, *PKQUEUE, *RESTRICTED_POINTER PRKQUEUE;

struct _KTHREAD;

typedef struct _KWAIT_BLOCK {
    LIST_ENTRY WaitListEntry;
    struct _KTHREAD *Thread;
    PVOID Object;
    struct _KWAIT_BLOCK *NextWaitBlock;
    SHORT WaitKey;
    SHORT WaitType;
} KWAIT_BLOCK, *PKWAIT_BLOCK;

typedef struct _KAPC {
    SHORT Type;
    CHAR ApcMode;
    UCHAR Inserted;
    struct _KTHREAD *Thread;
    LIST_ENTRY ApcListEntry;
    PVOID KernelRoutine;
    PVOID RundownRoutine;
    PVOID NormalRoutine;
    PVOID NormalContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
} KAPC, *PKAPC, *RESTRICTED_POINTER PRKAPC;

typedef struct _KSEMAPHORE {
    DISPATCHER_HEADER Header;
    LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE, *RESTRICTED_POINTER PRKSEMAPHORE;

typedef struct _KTHREAD {
    DISPATCHER_HEADER Header;
    LIST_ENTRY MutantListHead;
    ULONG KernelTime;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID KernelStack;
    PVOID TlsData;
    UCHAR State;
    UCHAR Alerted[2];
    UCHAR Alertable;
    UCHAR NpxState;
    CHAR Saturation;
    SCHAR Priority;
    UCHAR Padding;
    KAPC_STATE ApcState;
    ULONG ContextSwitches;
    LONG WaitStatus;
    UCHAR WaitIrql;
    CHAR WaitMode;
    UCHAR WaitNext;
    UCHAR WaitReason;
    PKWAIT_BLOCK WaitBlockList;
    LIST_ENTRY WaitListEntry;
    ULONG WaitTime;
    ULONG KernelApcDisable;
    LONG Quantum;
    SCHAR BasePriority;
    UCHAR DecrementCount;
    SCHAR PriorityDecrement;
    UCHAR DisableBoost;
    UCHAR NpxIrql;
    CHAR SuspendCount;
    UCHAR Preempted;
    UCHAR HasTerminated;
    PKQUEUE Queue;
    LIST_ENTRY QueueListEntry;
    KTIMER Timer;
    KWAIT_BLOCK TimerWaitBlock;
    KAPC SuspendApc;
    KSEMAPHORE SuspendSemaphore;
    LIST_ENTRY ThreadListEntry;
} KTHREAD, *PKTHREAD, *RESTRICTED_POINTER PRKTHREAD;

typedef enum _TIMER_TYPE {
    NotificationTimer,
    SynchronizationTimer
} TIMER_TYPE;

typedef struct _KINTERRUPT {
    PVOID ServiceRoutine;
    PVOID ServiceContext;
    ULONG BusInterruptLevel;
    ULONG Irql;
    UCHAR Connected;
    UCHAR ShareVector;
    UCHAR Mode;
    UCHAR Padding7;
    ULONG ServiceCount;
    ULONG DispatchCode[22];
} KINTERRUPT, *PKINTERRUPT;

typedef struct _KSYSTEM_TIME {
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME;

typedef enum _FIRMWARE_REENTRY {
    HalHaltRoutine,
    HalRebootRoutine,
    HalQuickRebootRoutine,
    HalKdRebootRoutine,
    HalFatalErrorRebootRoutine,
    HalMaximumRoutine
} FIRMWARE_REENTRY;

typedef struct _OBJECT_TYPE {
    PVOID AllocateProcedure;
    PVOID FreeProcedure;
    PVOID CloseProcedure;
    PVOID DeleteProcedure;
    PVOID ParseProcedure;
    PVOID DefaultObject;
    ULONG PoolTag;
} OBJECT_TYPE, *POBJECT_TYPE;

typedef struct _OBJECT_HEADER {
	LONG PointerCount;
	LONG HandleCount;
	POBJECT_TYPE Type;
	ULONG Flags;
	QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

#define OBJECT_TO_OBJECT_HEADER(Object) CONTAINING_RECORD(Object, OBJECT_HEADER, Body)

typedef VOID (STDCALL *PKDEFERRED_ROUTINE) (

    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

typedef struct _KDEVICE_QUEUE {
    SHORT Type;
    UCHAR Size;
    UCHAR Busy;
    LIST_ENTRY DeviceListHead;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE;

typedef VOID (STDCALL *PKSTART_ROUTINE) (

    IN PVOID StartContext
);

typedef union _FILE_SEGMENT_ELEMENT {
    PVOID Buffer;
    DWORD Alignment;
} FILE_SEGMENT_ELEMENT, *PFILE_SEGMENT_ELEMENT;

typedef VOID (STDCALL *PIO_APC_ROUTINE) (

    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
);

typedef struct _SEMAPHORE_BASIC_INFORMATION {
    LONG CurrentCount;
    LONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

typedef struct _MUTANT_BASIC_INFORMATION {
    LONG CurrentCount;
    UCHAR OwnedByCaller;
    UCHAR AbandonedState;
} MUTANT_BASIC_INFORMATION, *PMUTANT_BASIC_INFORMATION;

typedef struct _IO_COMPLETION_BASIC_INFORMATION {
    LONG Depth;
} IO_COMPLETION_BASIC_INFORMATION, *PIO_COMPLETION_BASIC_INFORMATION;

typedef struct _EVENT_BASIC_INFORMATION {
    EVENT_TYPE EventType;
    LONG EventState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

typedef struct _PS_STATISTICS {
    ULONG Length;
    ULONG ThreadCount;
    ULONG HandleCount;
} PS_STATISTICS, *PPS_STATISTICS;

typedef struct _ETHREAD {
    KTHREAD Tcb;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;

    union
    {
        NTSTATUS ExitStatus;
        PVOID OfsChain;
    };

    union
    {
        LIST_ENTRY ReaperListEntry;
        LIST_ENTRY ActiveTimerListHead;
    };

    HANDLE UniqueThread;
    PVOID StartAddress;
    LIST_ENTRY IrpList;
    PVOID DebugData;
} ETHREAD, *PETHREAD;

typedef VOID (STDCALL *PCREATE_THREAD_NOTIFY_ROUTINE) (

    IN PETHREAD Thread,
    IN HANDLE ThreadId,
    IN BOOLEAN Create
);

typedef enum _KINTERRUPT_MODE {
    LevelSensitive,
    Latched
} KINTERRUPT_MODE;

typedef BOOLEAN (STDCALL *PKSERVICE_ROUTINE) (

    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
);

typedef struct _TIMER_BASIC_INFORMATION {
    LARGE_INTEGER RemainingTime;
    BOOLEAN TimerState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

typedef VOID (STDCALL *PTIMER_APC_ROUTINE) (

    IN PVOID TimerContext,
    IN ULONG TimerLowValue,
    IN LONG TimerHighValue
);

typedef struct _XBOX_KRNL_VERSION {
    USHORT Major;
    USHORT Minor;
    USHORT Build;
    USHORT Qfe;
} XBOX_KRNL_VERSION, *PXBOX_KRNL_VERSION;

typedef struct _XBOX_HARDWARE_INFO {
    ULONG Flags;
    UCHAR GpuRevision;
    UCHAR McpRevision;
    UCHAR reserved[2];
} XBOX_HARDWARE_INFO;

#define XBOX_HW_FLAG_INTERNAL_USB_HUB 0x00000001
#define XBOX_HW_FLAG_DEVKIT_KERNEL    0x00000002
#define XBOX_480P_MACROVISION_ENABLED 0x00000004
#define XBOX_HW_FLAG_ARCADE           0x00000008
#define XBOX_KEY_LENGTH               16

typedef UCHAR XBOX_KEY_DATA[XBOX_KEY_LENGTH];

typedef struct _GENERIC_MAPPING {
    ACCESS_MASK GenericRead;
    ACCESS_MASK GenericWrite;
    ACCESS_MASK GenericExecute;
    ACCESS_MASK GenericAll;
} GENERIC_MAPPING, *PGENERIC_MAPPING;

typedef struct _SINGLE_LIST_ENTRY {
    struct _SINGLE_LIST_ENTRY *Next;

} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

typedef struct _SLIST_HEADER {
    ULONGLONG Alignment;
    struct
    {
        SINGLE_LIST_ENTRY Next;
        USHORT Depth;
        USHORT Sequence;
    };
} SLIST_HEADER, *PSLIST_HEADER;

typedef struct _KEVENT {
    DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT, *RESTRICTED_POINTER PRKEVENT;

typedef struct _KDEVICE_QUEUE_ENTRY {
    LIST_ENTRY DeviceListEntry;
    ULONG SortKey;
    BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY;

typedef struct _IO_COMPLETION_CONTEXT {
    PVOID Port;
    PVOID Key;
} IO_COMPLETION_CONTEXT, *PIO_COMPLETION_CONTEXT;

#endif
