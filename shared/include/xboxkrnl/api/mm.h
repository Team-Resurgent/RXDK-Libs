#ifndef XBOXKRNL_API_MM_H
#define XBOXKRNL_API_MM_H

XBAPI PVOID STDCALL MmAllocateContiguousMemory
(
    IN SIZE_T NumberOfBytes
);

XBAPI PVOID STDCALL MmAllocateContiguousMemoryEx
(
    IN SIZE_T NumberOfBytes,
    IN ULONG_PTR LowestAcceptableAddress,
    IN ULONG_PTR HighestAcceptableAddress,
    IN ULONG_PTR Alignment,
    IN ULONG Protect
);

XBAPI PVOID STDCALL MmAllocateSystemMemory
(
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect
);

XBAPI PVOID STDCALL MmClaimGpuInstanceMemory
(
    IN SIZE_T NumberOfBytes,
    OUT SIZE_T *NumberOfPaddingBytes
);

XBAPI PVOID STDCALL MmCreateKernelStack
(
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN DebuggerThread
);

XBAPI PVOID STDCALL MmDbgAllocateMemory
(
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect
);

XBAPI ULONG STDCALL MmDbgFreeMemory
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
);

XBAPI PFN_COUNT STDCALL MmDbgQueryAvailablePages (void);

XBAPI VOID STDCALL MmDbgReleaseAddress
(
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
);

XBAPI PVOID STDCALL MmDbgWriteCheck
(
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
);

XBAPI VOID STDCALL MmDeleteKernelStack
(
    IN PVOID KernelStackBase,
    IN PVOID KernelStackLimit
);

XBAPI VOID STDCALL MmFreeContiguousMemory
(
    IN PVOID BaseAddress
);

XBAPI ULONG STDCALL MmFreeSystemMemory
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
);

XBAPI ULONG_PTR STDCALL MmGetPhysicalAddress
(
    IN PVOID BaseAddress
);

XBAPI MMGLOBALDATA MmGlobalData;

XBAPI BOOLEAN STDCALL MmIsAddressValid
(
    IN PVOID VirtualAddress
);

XBAPI VOID STDCALL MmLockUnlockBufferPages
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN UnlockPages
);

XBAPI VOID STDCALL MmLockUnlockPhysicalPage
(
    IN ULONG_PTR PhysicalAddress,
    IN BOOLEAN UnlockPage
);

XBAPI PVOID STDCALL MmMapIoSpace
(
    IN ULONG_PTR PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG Protect
);

XBAPI VOID STDCALL MmPersistContiguousMemory
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN BOOLEAN Persist
);

XBAPI ULONG STDCALL MmQueryAddressProtect
(
    IN PVOID VirtualAddress
);

XBAPI SIZE_T STDCALL MmQueryAllocationSize
(
    IN PVOID BaseAddress
);

XBAPI NTSTATUS STDCALL MmQueryStatistics
(
    IN OUT PMM_STATISTICS MemoryStatistics
);

XBAPI VOID STDCALL MmSetAddressProtect
(
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytes,
    IN ULONG NewProtect
);

XBAPI PVOID STDCALL MmUnmapIoSpace
(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
);

#endif
