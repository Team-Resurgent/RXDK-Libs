#ifndef XBOXKRNL_API_MM_H
#define XBOXKRNL_API_MM_H

PVOID STDCALL MmAllocateContiguousMemory
(
    IN SIZE_T NumberOfBytes
);

PVOID STDCALL MmAllocateContiguousMemoryEx
(
    IN SIZE_T NumberOfBytes,
    IN ULONG_PTR LowestAcceptableAddress,
    IN ULONG_PTR HighestAcceptableAddress,
    IN ULONG_PTR Alignment,
    IN ULONG Protect
);

PVOID STDCALL MmAllocateSystemMemory
(
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect
);

PVOID STDCALL MmClaimGpuInstanceMemory
(
    IN SIZE_T NumberOfBytes,
    OUT SIZE_T *NumberOfPaddingBytes
);

PVOID STDCALL MmCreateKernelStack
(
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN DebuggerThread
);

PVOID STDCALL MmDbgAllocateMemory
(
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect
);

ULONG STDCALL MmDbgFreeMemory
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
);

PFN_COUNT STDCALL MmDbgQueryAvailablePages (void);

VOID STDCALL MmDbgReleaseAddress
(
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
);

PVOID STDCALL MmDbgWriteCheck
(
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
);

VOID STDCALL MmDeleteKernelStack
(
    IN PVOID KernelStackBase,
    IN PVOID KernelStackLimit
);

VOID STDCALL MmFreeContiguousMemory
(
    IN PVOID BaseAddress
);

ULONG STDCALL MmFreeSystemMemory
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
);

ULONG_PTR STDCALL MmGetPhysicalAddress
(
    IN PVOID BaseAddress
);

XBAPI MMGLOBALDATA MmGlobalData;

BOOLEAN STDCALL MmIsAddressValid
(
    IN PVOID VirtualAddress
);

VOID STDCALL MmLockUnlockBufferPages
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN UnlockPages
);

VOID STDCALL MmLockUnlockPhysicalPage
(
    IN ULONG_PTR PhysicalAddress,
    IN BOOLEAN UnlockPage
);

PVOID STDCALL MmMapIoSpace
(
    IN ULONG_PTR PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect
);

VOID STDCALL MmPersistContiguousMemory
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN Persist
);

ULONG STDCALL MmQueryAddressProtect
(
    IN PVOID VirtualAddress
);

SIZE_T STDCALL MmQueryAllocationSize
(
    IN PVOID BaseAddress
);

NTSTATUS STDCALL MmQueryStatistics
(
    IN OUT PMM_STATISTICS MemoryStatistics
);

VOID STDCALL MmSetAddressProtect
(
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytes,
    IN ULONG NewProtect
);

PVOID STDCALL MmUnmapIoSpace
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
);

#endif
