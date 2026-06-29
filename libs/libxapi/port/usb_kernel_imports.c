#include "bridge_usb.h"
/*
 * Pointers to xboxkrnl NTAPI exports for build/xapi_usb_kernel_wrap.c.
 */

#undef RtlZeroMemory
#undef RtlCopyMemory
#undef InitializeListHead

#include <xboxkrnl/xboxkrnl.h>

PVOID (__stdcall *const rxdk_krnl_ExAllocatePool)(SIZE_T) = &ExAllocatePool;
VOID (__stdcall *const rxdk_krnl_ExFreePool)(PVOID) = &ExFreePool;
VOID (__stdcall *const rxdk_krnl_KeInitializeTimerEx)(PKTIMER, TIMER_TYPE) = &KeInitializeTimerEx;
VOID (__stdcall *const rxdk_krnl_KeInitializeDpc)(KDPC *, PKDEFERRED_ROUTINE, PVOID) = &KeInitializeDpc;
BOOLEAN (__stdcall *const rxdk_krnl_KeSetTimer)(PKTIMER, LARGE_INTEGER, PKDPC) = &KeSetTimer;
BOOLEAN (__stdcall *const rxdk_krnl_KeCancelTimer)(PKTIMER) = &KeCancelTimer;
KIRQL (__stdcall *const rxdk_krnl_KeRaiseIrqlToDpcLevel)(void) = &KeRaiseIrqlToDpcLevel;
NTSTATUS (__stdcall *const rxdk_krnl_KeWaitForSingleObject)(PVOID, KWAIT_REASON, KPROCESSOR_MODE, BOOLEAN, PLARGE_INTEGER) = &KeWaitForSingleObject;
LONG (__stdcall *const rxdk_krnl_KeSetEvent)(PRKEVENT, KPRIORITY, BOOLEAN) = &KeSetEvent;
NTSTATUS (__stdcall *const rxdk_krnl_ObReferenceObjectByHandle)(HANDLE, POBJECT_TYPE, PVOID *) = &ObReferenceObjectByHandle;
ULONGLONG (__stdcall *const rxdk_krnl_KeQueryInterruptTime)(void) = &KeQueryInterruptTime;
NTSTATUS (__stdcall *const rxdk_krnl_IoInvalidDeviceRequest)(PDEVICE_OBJECT, PIRP) = &IoInvalidDeviceRequest;
NTSTATUS (__stdcall *const rxdk_krnl_IoCreateDevice)(PDRIVER_OBJECT, ULONG, POBJECT_STRING, DEVICE_TYPE, BOOLEAN, PDEVICE_OBJECT *) = &IoCreateDevice;
ULONG (__stdcall *const rxdk_krnl_HalGetInterruptVector)(ULONG, PKIRQL) = &HalGetInterruptVector;
VOID (__stdcall *const rxdk_krnl_KeStallExecutionProcessor)(ULONG) = &KeStallExecutionProcessor;
VOID (__stdcall *const rxdk_krnl_KeInitializeInterrupt)(PKINTERRUPT, PKSERVICE_ROUTINE, PVOID, ULONG, KIRQL, KINTERRUPT_MODE, BOOLEAN) = &KeInitializeInterrupt;
BOOLEAN (__stdcall *const rxdk_krnl_KeConnectInterrupt)(PKINTERRUPT) = &KeConnectInterrupt;
VOID (__stdcall *const rxdk_krnl_HalRegisterShutdownNotification)(PHAL_SHUTDOWN_REGISTRATION, BOOLEAN) = &HalRegisterShutdownNotification;
VOID (__stdcall *const rxdk_krnl_IoStartPacket)(PDEVICE_OBJECT, PIRP, PULONG) = &IoStartPacket;
BOOLEAN (__stdcall *const rxdk_krnl_HalIsResetOrShutdownPending)(void) = &HalIsResetOrShutdownPending;
VOID (__stdcall *const rxdk_krnl_IoStartNextPacket)(PDEVICE_OBJECT) = &IoStartNextPacket;
VOID (__stdcall *const rxdk_krnl_IoMarkIrpMustComplete)(PIRP) = &IoMarkIrpMustComplete;
ULONG (__stdcall *const rxdk_krnl_MmQueryAddressProtect)(PVOID) = &MmQueryAddressProtect;
VOID (__stdcall *const rxdk_krnl_MmSetAddressProtect)(PVOID, ULONG, ULONG) = &MmSetAddressProtect;
PVOID (__stdcall *const rxdk_krnl_MmAllocateContiguousMemory)(SIZE_T) = &MmAllocateContiguousMemory;
VOID (__stdcall *const rxdk_krnl_MmLockUnlockBufferPages)(PVOID, SIZE_T, BOOLEAN) = &MmLockUnlockBufferPages;
ULONG_PTR (__stdcall *const rxdk_krnl_MmGetPhysicalAddress)(PVOID) = &MmGetPhysicalAddress;
NTSTATUS (__stdcall *const rxdk_krnl_KeDelayExecutionThread)(KPROCESSOR_MODE, BOOLEAN, PLARGE_INTEGER) = &KeDelayExecutionThread;
BOOLEAN (__stdcall *const rxdk_krnl_KeInsertQueueDpc)(PRKDPC, PVOID, PVOID) = &KeInsertQueueDpc;
VOID (__stdcall *const rxdk_krnl_MmLockUnlockPhysicalPage)(ULONG_PTR, BOOLEAN) = &MmLockUnlockPhysicalPage;

/* Additional kernel exports called by libdsound (MCPX APU driver). Same
 * cdecl-facade bridging as above: dsound's vendor ntos.h calls are cdecl,
 * xboxkrnl is __stdcall. */
ULONG (__stdcall *const rxdk_krnl_ExQueryPoolBlockSize)(PVOID) = &ExQueryPoolBlockSize;
BOOLEAN (__stdcall *const rxdk_krnl_KeDisconnectInterrupt)(PKINTERRUPT) = &KeDisconnectInterrupt;
VOID (__stdcall *const rxdk_krnl_KeQuerySystemTime)(PLARGE_INTEGER) = &KeQuerySystemTime;
BOOLEAN (__stdcall *const rxdk_krnl_KeRemoveQueueDpc)(PRKDPC) = &KeRemoveQueueDpc;
BOOLEAN (__stdcall *const rxdk_krnl_KeSynchronizeExecution)(PKINTERRUPT, PKSYNCHRONIZE_ROUTINE, PVOID) = &KeSynchronizeExecution;
PVOID (__stdcall *const rxdk_krnl_MmAllocateContiguousMemoryEx)(SIZE_T, ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG) = &MmAllocateContiguousMemoryEx;
VOID (__stdcall *const rxdk_krnl_MmFreeContiguousMemory)(PVOID) = &MmFreeContiguousMemory;
SIZE_T (__stdcall *const rxdk_krnl_MmQueryAllocationSize)(PVOID) = &MmQueryAllocationSize;
