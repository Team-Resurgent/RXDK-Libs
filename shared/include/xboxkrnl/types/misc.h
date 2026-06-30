#ifndef XBOXKRNL_TYPES_MISC_H
#define XBOXKRNL_TYPES_MISC_H

typedef struct _OBJECT_DIRECTORY_INFORMATION {
    OBJECT_STRING Name;
    ULONG Type;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

typedef struct _ERWLOCK {
    LONG LockCount;
    ULONG WritersWaitingCount;
    ULONG ReadersWaitingCount;
    ULONG ReadersEntryCount;
    KEVENT WriterEvent;
    KSEMAPHORE ReaderSemaphore;
} ERWLOCK, *PERWLOCK;

typedef struct _XBOX_REFURB_INFO {
    ULONG Signature;
    ULONG PowerCycleCount;
    LARGE_INTEGER FirstSetTime;
} XBOX_REFURB_INFO;

typedef struct _KFLOATING_SAVE {
    ULONG ControlWord;
    ULONG StatusWord;
    ULONG ErrorOffset;
    ULONG ErrorSelector;
    ULONG DataOffset;
    ULONG DataSelector;
    ULONG Cr0NpxState;
    ULONG Spare1;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

typedef struct _HARDWARE_PTE {
    ULONG Valid : 1;
    ULONG Write : 1;
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Accessed : 1;
    ULONG Dirty : 1;
    ULONG LargePage : 1;
    ULONG Global : 1;
    ULONG GuardOrEndOfAllocation : 1;
    ULONG PersistAllocation : 1;
    ULONG reserved : 1;
    ULONG PageFrameNumber : 20;
} HARDWARE_PTE, *PHARDWARE_PTE;

typedef VOID (STDCALL *PPS_APC_ROUTINE) (

    IN PVOID ApcArgument1,
    IN PVOID ApcArgument2,
    IN PVOID ApcArgument3
);

typedef enum _KWAIT_REASON {
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrPoolAllocation,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrEventPair,
    WrQueue,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    WrRendezvous,
    WrFsCacheIn,
    WrFsCacheOut,
    Spare4,
    Spare5,
    Spare6,
    WrKernel,
    MaximumWaitReason
} KWAIT_REASON;

typedef struct _KMUTANT {
    DISPATCHER_HEADER Header;
    LIST_ENTRY MutantListEntry;
    PRKTHREAD OwnerThread;
    BOOLEAN Abandoned;
} KMUTANT, *PKMUTANT, *RESTRICTED_POINTER PRKMUTANT;

typedef struct _SHARE_ACCESS {
    UCHAR OpenCount;
    UCHAR Readers;
    UCHAR Writers;
    UCHAR Deleters;
    UCHAR SharedRead;
    UCHAR SharedWrite;
    UCHAR SharedDelete;
} SHARE_ACCESS, *PSHARE_ACCESS;

typedef BOOLEAN (STDCALL *PKSYNCHRONIZE_ROUTINE) (

    IN PVOID SynchronizeContext
);

typedef VOID (STDCALL *PKRUNDOWN_ROUTINE) (

    IN PKAPC Apc
);

typedef VOID (STDCALL *PKNORMAL_ROUTINE) (

    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

typedef VOID (STDCALL *PKKERNEL_ROUTINE) (

    IN PKAPC Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
);

struct _HAL_SHUTDOWN_REGISTRATION;

typedef VOID (STDCALL *PHAL_SHUTDOWN_NOTIFICATION) (

    IN struct _HAL_SHUTDOWN_REGISTRATION *ShutdownRegistration
);

typedef struct _HAL_SHUTDOWN_REGISTRATION {
    PHAL_SHUTDOWN_NOTIFICATION NotificationRoutine;
    LONG Priority;
    LIST_ENTRY ListEntry;
} HAL_SHUTDOWN_REGISTRATION, *PHAL_SHUTDOWN_REGISTRATION;

typedef VOID (STDCALL *pfXcSHAInit) (PUCHAR pbSHAContext);
typedef VOID (STDCALL *pfXcSHAUpdate) (PUCHAR pbSHAContext, PUCHAR pbInput, ULONG dwInputLength);
typedef VOID (STDCALL *pfXcSHAFinal) (PUCHAR pbSHAContext, PUCHAR pbDigest);
typedef VOID (STDCALL *pfXcRC4Key) (PUCHAR pbKeyStruct, ULONG dwKeyLength, PUCHAR pbKey);
typedef VOID (STDCALL *pfXcRC4Crypt) (PUCHAR pbKeyStruct, ULONG dwInputLength, PUCHAR pbInput);
typedef VOID (STDCALL *pfXcHMAC) (PUCHAR pbKey, ULONG dwKeyLength, PUCHAR pbInput, ULONG dwInputLength, PUCHAR pbInput2, ULONG dwInputLength2, PUCHAR pbDigest);
typedef ULONG (STDCALL *pfXcPKEncPublic) (PUCHAR pbPubKey, PUCHAR pbInput, PUCHAR pbOutput);
typedef ULONG (STDCALL *pfXcPKDecPrivate) (PUCHAR pbPrvKey, PUCHAR pbInput, PUCHAR pbOutput);
typedef ULONG (STDCALL *pfXcPKGetKeyLen) (PUCHAR pbPubKey);
typedef BOOLEAN (STDCALL *pfXcVerifyPKCS1Signature) (PUCHAR pbSig, PUCHAR pbPubKey, PUCHAR pbDigest);
typedef ULONG (STDCALL *pfXcModExp) (PULONG pA, PULONG pB, PULONG pC, PULONG pD, ULONG dwN);
typedef VOID (STDCALL *pfXcDESKeyParity) (PUCHAR pbKey, ULONG dwKeyLength);
typedef VOID (STDCALL *pfXcKeyTable) (ULONG dwCipher, PUCHAR pbKeyTable, PUCHAR pbKey);
typedef VOID (STDCALL *pfXcBlockCrypt) (ULONG dwCipher, PUCHAR pbOutput, PUCHAR pbInput, PUCHAR pbKeyTable, ULONG dwOp);
typedef VOID (STDCALL *pfXcBlockCryptCBC) (ULONG dwCipher, ULONG dwInputLength, PUCHAR pbOutput, PUCHAR pbInput, PUCHAR pbKeyTable, ULONG dwOp, PUCHAR pbFeedback);
typedef ULONG (STDCALL *pfXcCryptService) (ULONG dwOp, PVOID pArgs);

typedef struct {
    pfXcSHAInit pXcSHAInit;
    pfXcSHAUpdate pXcSHAUpdate;
    pfXcSHAFinal pXcSHAFinal;
    pfXcRC4Key pXcRC4Key;
    pfXcRC4Crypt pXcRC4Crypt;
    pfXcHMAC pXcHMAC;
    pfXcPKEncPublic pXcPKEncPublic;
    pfXcPKDecPrivate pXcPKDecPrivate;
    pfXcPKGetKeyLen pXcPKGetKeyLen;
    pfXcVerifyPKCS1Signature pXcVerifyPKCS1Signature;
    pfXcModExp pXcModExp;
    pfXcDESKeyParity pXcDESKeyParity;
    pfXcKeyTable pXcKeyTable;
    pfXcBlockCrypt pXcBlockCrypt;
    pfXcBlockCryptCBC pXcBlockCryptCBC;
    pfXcCryptService pXcCryptService;
} CRYPTO_VECTOR, *PCRYPTO_VECTOR;

typedef struct _OBJECT_HANDLE_TABLE {
    LONG HandleCount;
    LONG_PTR FirstFreeTableEntry;
    HANDLE NextHandleNeedingPool;
    PVOID **RootTable;
    PVOID *BuiltinRootTable[8];
} OBJECT_HANDLE_TABLE, *POBJECT_HANDLE_TABLE;

typedef struct _MMPFNFREE {
    USHORT PackedPfnFlink;
    USHORT PackedPfnBlink;
} MMPFNFREE, *PMMPFNFREE;

typedef struct _MMPFNREGION {
    MMPFNFREE FreePagesByColor[32];
    PFN_COUNT AvailablePages;
} MMPFNREGION, *PMMPFNREGION;

typedef struct _MMPTE {
    union
    {
        ULONG Long;
        HARDWARE_PTE Hard;
        struct
        {
            ULONG Valid : 1;
            ULONG OneEntry : 1;
            ULONG NextEntry : 30;
        } List;
    };
} MMPTE, *PMMPTE;

typedef enum _MMPFN_BUSY_TYPE {
    MmUnknownUsage,
    MmStackUsage,
    MmVirtualPageTableUsage,
    MmSystemPageTableUsage,
    MmPoolUsage,
    MmVirtualMemoryUsage,
    MmSystemMemoryUsage,
    MmImageUsage,
    MmFsCacheUsage,
    MmContiguousUsage,
    MmDebuggerUsage,
    MmMaximumUsage
} MMPFN_BUSY_TYPE;

typedef PFN_NUMBER (FASTCALL *PMMREMOVE_PAGE_ROUTINE) (

    IN MMPFN_BUSY_TYPE BusyType,
    IN PMMPTE TargetPte
);

typedef struct _MMPTERANGE {
    MMPTE HeadPte;
    PMMPTE FirstCommittedPte;
    PMMPTE LastCommittedPte;
    PMMPTE LastReservedPte;
    PFN_COUNT *AvailablePages;
    PMMREMOVE_PAGE_ROUTINE RemovePageRoutine;
} MMPTERANGE, *PMMPTERANGE;

typedef struct _MMADDRESS_NODE {
    ULONG_PTR StartingVpn;
    ULONG_PTR EndingVpn;
    struct _MMADDRESS_NODE *Parent;
    struct _MMADDRESS_NODE *LeftChild;
    struct _MMADDRESS_NODE *RightChild;

} MMADDRESS_NODE, *PMMADDRESS_NODE;

typedef struct _MMGLOBALDATA {
    PMMPFNREGION RetailPfnRegion;
    PMMPTERANGE SystemPteRange;
    PULONG AvailablePages;
    PFN_COUNT *AllocatedPagesByUsage;
    PRTL_CRITICAL_SECTION AddressSpaceLock;
    PMMADDRESS_NODE *VadRoot;
    PMMADDRESS_NODE *VadHint;
    PMMADDRESS_NODE *VadFreeHint;
} MMGLOBALDATA, *PMMGLOBALDATA;

typedef VOID (STDCALL *PIDE_INTERRUPT_ROUTINE) (void);
typedef VOID (STDCALL *PIDE_FINISHIO_ROUTINE) (void);
typedef BOOLEAN (STDCALL *PIDE_POLL_RESET_COMPLETE_ROUTINE) (void);
typedef VOID (STDCALL *PIDE_TIMEOUT_EXPIRED_ROUTINE) (void);

typedef VOID (STDCALL *PIDE_START_PACKET_ROUTINE) (

    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

typedef VOID (STDCALL *PIDE_START_NEXT_PACKET_ROUTINE) (void);

typedef struct _IDE_CHANNEL_OBJECT {
    PIDE_INTERRUPT_ROUTINE InterruptRoutine;
    PIDE_FINISHIO_ROUTINE FinishIoRoutine;
    PIDE_POLL_RESET_COMPLETE_ROUTINE PollResetCompleteRoutine;
    PIDE_TIMEOUT_EXPIRED_ROUTINE TimeoutExpiredRoutine;
    PIDE_START_PACKET_ROUTINE StartPacketRoutine;
    PIDE_START_NEXT_PACKET_ROUTINE StartNextPacketRoutine;
    KIRQL InterruptIrql;
    BOOLEAN ExpectingBusMasterInterrupt;
    BOOLEAN StartPacketBusy;
    BOOLEAN StartPacketRequested;
    UCHAR Timeout;
    UCHAR IoRetries;
    UCHAR MaximumIoRetries;
    PIRP CurrentIrp;
    KDEVICE_QUEUE DeviceQueue;
    ULONG PhysicalRegionDescriptorTablePhysical;
    KDPC TimerDpc;
    KDPC FinishDpc;
    KTIMER Timer;
    KINTERRUPT InterruptObject;
} IDE_CHANNEL_OBJECT, *PIDE_CHANNEL_OBJECT;

typedef VOID (STDCALL *PKSYSTEM_ROUTINE) (

    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext OPTIONAL
);

#define LOW_PRIORITY          0
#define LOW_REALTIME_PRIORITY 16
#define HIGH_PRIORITY         31
#define MAXIMUM_PRIORITY      32

#endif
