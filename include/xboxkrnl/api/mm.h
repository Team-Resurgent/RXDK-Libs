#ifndef XBOXKRNL_API_MM_H
#define XBOXKRNL_API_MM_H

XBAPI PVOID NTAPI MmAllocateContiguousMemory
(
    IN SIZE_T NumberOfBytes
);

XBAPI PVOID NTAPI MmAllocateContiguousMemoryEx
(
    IN SIZE_T NumberOfBytes,
    IN ULONG_PTR LowestAcceptableAddress,
    IN ULONG_PTR HighestAcceptableAddress,
    IN ULONG_PTR Alignment,
    IN ULONG Protect
);

XBAPI PVOID NTAPI MmAllocateSystemMemory
(
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect
);

XBAPI PVOID NTAPI MmClaimGpuInstanceMemory
(
    IN SIZE_T NumberOfBytes,
    OUT SIZE_T *NumberOfPaddingBytes
);

XBAPI PVOID NTAPI MmCreateKernelStack
(
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN DebuggerThread
);

XBAPI PVOID NTAPI MmDbgAllocateMemory
(
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect
);

XBAPI ULONG NTAPI MmDbgFreeMemory
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
);

XBAPI PFN_COUNT NTAPI MmDbgQueryAvailablePages (void);

XBAPI VOID NTAPI MmDbgReleaseAddress
(
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
);

XBAPI PVOID NTAPI MmDbgWriteCheck
(
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
);

XBAPI VOID NTAPI MmDeleteKernelStack
(
    IN PVOID KernelStackBase,
    IN PVOID KernelStackLimit
);

XBAPI VOID NTAPI MmFreeContiguousMemory
(
    IN PVOID BaseAddress
);

XBAPI ULONG NTAPI MmFreeSystemMemory
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
);

XBAPI ULONG_PTR NTAPI MmGetPhysicalAddress
(
    IN PVOID BaseAddress
);

XBAPI MMGLOBALDATA MmGlobalData;

XBAPI BOOLEAN NTAPI MmIsAddressValid
(
    IN PVOID VirtualAddress
);

XBAPI VOID NTAPI MmLockUnlockBufferPages
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN UnlockPages
);

XBAPI VOID NTAPI MmLockUnlockPhysicalPage
(
    IN ULONG_PTR PhysicalAddress,
    IN BOOLEAN UnlockPage
);

XBAPI PVOID NTAPI MmMapIoSpace
(
    IN ULONG_PTR PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect
);

XBAPI VOID NTAPI MmPersistContiguousMemory
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN Persist
);

XBAPI ULONG NTAPI MmQueryAddressProtect
(
    IN PVOID VirtualAddress
);

XBAPI SIZE_T NTAPI MmQueryAllocationSize
(
    IN PVOID BaseAddress
);

XBAPI NTSTATUS NTAPI MmQueryStatistics
(
    IN OUT PMM_STATISTICS MemoryStatistics
);

XBAPI VOID NTAPI MmSetAddressProtect
(
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytes,
    IN ULONG NewProtect
);

XBAPI PVOID NTAPI MmUnmapIoSpace
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
);

#endif
