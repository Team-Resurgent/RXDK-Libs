#include "bridge_usb.h"
/*
 * Cdecl facades for vendor ntos.h kernel calls (USB / OHCD objects).
 * xboxkrnl exports are NTAPI (__stdcall @N on x86).
 */

#undef RtlZeroMemory
#undef RtlCopyMemory
#undef InitializeListHead

#include <ntos.h>

extern PVOID (__stdcall *const rxdk_krnl_ExAllocatePool)(SIZE_T);
extern VOID (__stdcall *const rxdk_krnl_ExFreePool)(PVOID);
extern VOID (__stdcall *const rxdk_krnl_KeInitializeTimerEx)(PKTIMER, TIMER_TYPE);
extern VOID (__stdcall *const rxdk_krnl_KeInitializeDpc)(KDPC *, PKDEFERRED_ROUTINE, PVOID);
extern BOOLEAN (__stdcall *const rxdk_krnl_KeSetTimer)(PKTIMER, LARGE_INTEGER, PKDPC);
extern BOOLEAN (__stdcall *const rxdk_krnl_KeCancelTimer)(PKTIMER);
extern KIRQL (__stdcall *const rxdk_krnl_KeRaiseIrqlToDpcLevel)(void);
extern NTSTATUS (__stdcall *const rxdk_krnl_KeWaitForSingleObject)(PVOID, KWAIT_REASON, KPROCESSOR_MODE, BOOLEAN, PLARGE_INTEGER);
extern LONG (__stdcall *const rxdk_krnl_KeSetEvent)(PRKEVENT, KPRIORITY, BOOLEAN);
extern NTSTATUS (__stdcall *const rxdk_krnl_ObReferenceObjectByHandle)(HANDLE, POBJECT_TYPE, PVOID *);
extern ULONGLONG (__stdcall *const rxdk_krnl_KeQueryInterruptTime)(void);
extern NTSTATUS (__stdcall *const rxdk_krnl_IoInvalidDeviceRequest)(PDEVICE_OBJECT, PIRP);
extern NTSTATUS (__stdcall *const rxdk_krnl_IoCreateDevice)(PDRIVER_OBJECT, ULONG, POBJECT_STRING, DEVICE_TYPE, BOOLEAN, PDEVICE_OBJECT *);
extern ULONG (__stdcall *const rxdk_krnl_HalGetInterruptVector)(ULONG, PKIRQL);
extern VOID (__stdcall *const rxdk_krnl_KeStallExecutionProcessor)(ULONG);
extern VOID (__stdcall *const rxdk_krnl_KeInitializeInterrupt)(PKINTERRUPT, PKSERVICE_ROUTINE, PVOID, ULONG, KIRQL, KINTERRUPT_MODE, BOOLEAN);
extern BOOLEAN (__stdcall *const rxdk_krnl_KeConnectInterrupt)(PKINTERRUPT);
extern VOID (__stdcall *const rxdk_krnl_HalRegisterShutdownNotification)(PHAL_SHUTDOWN_REGISTRATION, BOOLEAN);
extern VOID (__stdcall *const rxdk_krnl_IoStartPacket)(PDEVICE_OBJECT, PIRP, PULONG);
extern BOOLEAN (__stdcall *const rxdk_krnl_HalIsResetOrShutdownPending)(void);
extern VOID (__stdcall *const rxdk_krnl_IoStartNextPacket)(PDEVICE_OBJECT);
extern VOID (__stdcall *const rxdk_krnl_IoMarkIrpMustComplete)(PIRP);
extern ULONG (__stdcall *const rxdk_krnl_MmQueryAddressProtect)(PVOID);
extern VOID (__stdcall *const rxdk_krnl_MmSetAddressProtect)(PVOID, ULONG, ULONG);
extern PVOID (__stdcall *const rxdk_krnl_MmAllocateContiguousMemory)(SIZE_T);
extern VOID (__stdcall *const rxdk_krnl_MmLockUnlockBufferPages)(PVOID, SIZE_T, BOOLEAN);
extern ULONG_PTR (__stdcall *const rxdk_krnl_MmGetPhysicalAddress)(PVOID);
extern NTSTATUS (__stdcall *const rxdk_krnl_KeDelayExecutionThread)(KPROCESSOR_MODE, BOOLEAN, PLARGE_INTEGER);
extern BOOLEAN (__stdcall *const rxdk_krnl_KeInsertQueueDpc)(PRKDPC, PVOID, PVOID);
extern VOID (__stdcall *const rxdk_krnl_MmLockUnlockPhysicalPage)(ULONG_PTR, BOOLEAN);

PVOID __cdecl ExAllocatePool(SIZE_T NumberOfBytes)
{
    return rxdk_krnl_ExAllocatePool(NumberOfBytes);
}

VOID __cdecl ExFreePool(PVOID P)
{
    rxdk_krnl_ExFreePool(P);
}

VOID __cdecl KeInitializeTimerEx(PKTIMER Timer, TIMER_TYPE Type)
{
    rxdk_krnl_KeInitializeTimerEx(Timer, Type);
}

VOID __cdecl KeInitializeDpc(PRKDPC Dpc, PKDEFERRED_ROUTINE DeferredRoutine, PVOID DeferredContext)
{
    rxdk_krnl_KeInitializeDpc(Dpc, DeferredRoutine, DeferredContext);
}

BOOLEAN __cdecl KeSetTimer(PKTIMER Timer, LARGE_INTEGER DueTime, PKDPC Dpc)
{
    return rxdk_krnl_KeSetTimer(Timer, DueTime, Dpc);
}

BOOLEAN __cdecl KeCancelTimer(PKTIMER Timer)
{
    return rxdk_krnl_KeCancelTimer(Timer);
}

KIRQL __cdecl KeRaiseIrqlToDpcLevel(void)
{
    return rxdk_krnl_KeRaiseIrqlToDpcLevel();
}

NTSTATUS __cdecl KeWaitForSingleObject(PVOID Object, KWAIT_REASON WaitReason, KPROCESSOR_MODE WaitMode, BOOLEAN Alertable, PLARGE_INTEGER Timeout)
{
    return rxdk_krnl_KeWaitForSingleObject(Object, WaitReason, WaitMode, Alertable, Timeout);
}

LONG __cdecl KeSetEvent(PRKEVENT Event, KPRIORITY Increment, BOOLEAN Wait)
{
    return rxdk_krnl_KeSetEvent(Event, Increment, Wait);
}

NTSTATUS __cdecl ObReferenceObjectByHandle(HANDLE Handle, POBJECT_TYPE ObjectType, PVOID *Object)
{
    return rxdk_krnl_ObReferenceObjectByHandle(Handle, ObjectType, Object);
}

ULONGLONG __cdecl KeQueryInterruptTime(void)
{
    return rxdk_krnl_KeQueryInterruptTime();
}

NTSTATUS __cdecl IoInvalidDeviceRequest(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return rxdk_krnl_IoInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS __cdecl IoCreateDevice(PDRIVER_OBJECT DriverObject, ULONG DeviceExtensionSize, POBJECT_STRING DeviceName, DEVICE_TYPE DeviceType, BOOLEAN Exclusive, PDEVICE_OBJECT *DeviceObject)
{
    return rxdk_krnl_IoCreateDevice(DriverObject, DeviceExtensionSize, DeviceName, DeviceType, Exclusive, DeviceObject);
}

ULONG __cdecl HalGetInterruptVector(ULONG BusInterruptLevel, PKIRQL Irql)
{
    return rxdk_krnl_HalGetInterruptVector(BusInterruptLevel, Irql);
}

VOID __cdecl KeStallExecutionProcessor(ULONG MicroSeconds)
{
    rxdk_krnl_KeStallExecutionProcessor(MicroSeconds);
}

VOID __cdecl KeInitializeInterrupt(PKINTERRUPT Interrupt, PKSERVICE_ROUTINE ServiceRoutine, PVOID ServiceContext, ULONG Vector, KIRQL Irql, KINTERRUPT_MODE InterruptMode, BOOLEAN ShareVector)
{
    rxdk_krnl_KeInitializeInterrupt(Interrupt, ServiceRoutine, ServiceContext, Vector, Irql, InterruptMode, ShareVector);
}

BOOLEAN __cdecl KeConnectInterrupt(PKINTERRUPT Interrupt)
{
    return rxdk_krnl_KeConnectInterrupt(Interrupt);
}

VOID __cdecl HalRegisterShutdownNotification(PHAL_SHUTDOWN_REGISTRATION ShutdownRegistration, BOOLEAN Register)
{
    rxdk_krnl_HalRegisterShutdownNotification(ShutdownRegistration, Register);
}

VOID __cdecl IoStartPacket(PDEVICE_OBJECT DeviceObject, PIRP Irp, PULONG Key)
{
    rxdk_krnl_IoStartPacket(DeviceObject, Irp, Key);
}

BOOLEAN __cdecl HalIsResetOrShutdownPending(void)
{
    return rxdk_krnl_HalIsResetOrShutdownPending();
}

VOID __cdecl IoStartNextPacket(PDEVICE_OBJECT DeviceObject)
{
    rxdk_krnl_IoStartNextPacket(DeviceObject);
}

VOID __cdecl IoMarkIrpMustComplete(PIRP Irp)
{
    rxdk_krnl_IoMarkIrpMustComplete(Irp);
}

ULONG __cdecl MmQueryAddressProtect(PVOID VirtualAddress)
{
    return rxdk_krnl_MmQueryAddressProtect(VirtualAddress);
}

VOID __cdecl MmSetAddressProtect(PVOID BaseAddress, ULONG NumberOfBytes, ULONG NewProtect)
{
    rxdk_krnl_MmSetAddressProtect(BaseAddress, NumberOfBytes, NewProtect);
}

PVOID __cdecl MmAllocateContiguousMemory(SIZE_T NumberOfBytes)
{
    return rxdk_krnl_MmAllocateContiguousMemory(NumberOfBytes);
}

VOID __cdecl MmLockUnlockBufferPages(PVOID BaseAddress, SIZE_T NumberOfBytes, BOOLEAN UnlockPages)
{
    rxdk_krnl_MmLockUnlockBufferPages(BaseAddress, NumberOfBytes, UnlockPages);
}

ULONG_PTR __cdecl MmGetPhysicalAddress(PVOID BaseAddress)
{
    return rxdk_krnl_MmGetPhysicalAddress(BaseAddress);
}

NTSTATUS __cdecl KeDelayExecutionThread(KPROCESSOR_MODE WaitMode, BOOLEAN Alertable, PLARGE_INTEGER Interval)
{
    return rxdk_krnl_KeDelayExecutionThread(WaitMode, Alertable, Interval);
}

BOOLEAN __cdecl KeInsertQueueDpc(PRKDPC Dpc, PVOID SystemArgument1, PVOID SystemArgument2)
{
    return rxdk_krnl_KeInsertQueueDpc(Dpc, SystemArgument1, SystemArgument2);
}

VOID __cdecl MmLockUnlockPhysicalPage(ULONG_PTR PhysicalAddress, BOOLEAN UnlockPage)
{
    rxdk_krnl_MmLockUnlockPhysicalPage(PhysicalAddress, UnlockPage);
}

/*
 * Additional cdecl facades for libdsound (MCPX APU driver). These kernel
 * exports were previously stubbed via prebuilt/xboxkrnl_dsound_imports.lib,
 * which provided undecorated imports straight to the __stdcall kernel -- an
 * ABI mismatch (cdecl caller vs stdcall callee). Routing through the stdcall
 * function pointers fixes the convention, same as the USB facades above.
 */
extern ULONG (__stdcall *const rxdk_krnl_ExQueryPoolBlockSize)(PVOID);
extern BOOLEAN (__stdcall *const rxdk_krnl_KeDisconnectInterrupt)(PKINTERRUPT);
extern VOID (__stdcall *const rxdk_krnl_KeQuerySystemTime)(PLARGE_INTEGER);
extern BOOLEAN (__stdcall *const rxdk_krnl_KeRemoveQueueDpc)(PRKDPC);
extern BOOLEAN (__stdcall *const rxdk_krnl_KeSynchronizeExecution)(PKINTERRUPT, PKSYNCHRONIZE_ROUTINE, PVOID);
extern PVOID (__stdcall *const rxdk_krnl_MmAllocateContiguousMemoryEx)(SIZE_T, ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG);
extern VOID (__stdcall *const rxdk_krnl_MmFreeContiguousMemory)(PVOID);
extern SIZE_T (__stdcall *const rxdk_krnl_MmQueryAllocationSize)(PVOID);

ULONG __cdecl ExQueryPoolBlockSize(PVOID PoolBlock)
{
    return rxdk_krnl_ExQueryPoolBlockSize(PoolBlock);
}

BOOLEAN __cdecl KeDisconnectInterrupt(PKINTERRUPT Interrupt)
{
    return rxdk_krnl_KeDisconnectInterrupt(Interrupt);
}

VOID __cdecl KeQuerySystemTime(PLARGE_INTEGER CurrentTime)
{
    rxdk_krnl_KeQuerySystemTime(CurrentTime);
}

BOOLEAN __cdecl KeRemoveQueueDpc(PRKDPC Dpc)
{
    return rxdk_krnl_KeRemoveQueueDpc(Dpc);
}

BOOLEAN __cdecl KeSynchronizeExecution(PKINTERRUPT Interrupt, PKSYNCHRONIZE_ROUTINE SynchronizeRoutine, PVOID SynchronizeContext)
{
    return rxdk_krnl_KeSynchronizeExecution(Interrupt, SynchronizeRoutine, SynchronizeContext);
}

PVOID __cdecl MmAllocateContiguousMemoryEx(SIZE_T NumberOfBytes, ULONG_PTR LowestAcceptableAddress, ULONG_PTR HighestAcceptableAddress, ULONG_PTR Alignment, ULONG Protect)
{
    return rxdk_krnl_MmAllocateContiguousMemoryEx(NumberOfBytes, LowestAcceptableAddress, HighestAcceptableAddress, Alignment, Protect);
}

VOID __cdecl MmFreeContiguousMemory(PVOID BaseAddress)
{
    rxdk_krnl_MmFreeContiguousMemory(BaseAddress);
}

SIZE_T __cdecl MmQueryAllocationSize(PVOID BaseAddress)
{
    return rxdk_krnl_MmQueryAllocationSize(BaseAddress);
}

/*
 * Additional cdecl facades for libxnet (TCP fast/slow timers + the MCPX
 * ethernet PHY). Same cdecl->stdcall bridging as above. PhyInitialize /
 * PhyGetLinkState are kernel exports (ordinals 253 / 252); the xnet source's
 * own phy.c copies are gated out in our build.
 */
extern BOOLEAN (__stdcall *const rxdk_krnl_KeSetTimerEx)(PKTIMER, LARGE_INTEGER, LONG, PKDPC);
extern NTSTATUS (__stdcall *const rxdk_krnl_PhyInitialize)(BOOLEAN, PVOID);
extern ULONG (__stdcall *const rxdk_krnl_PhyGetLinkState)(BOOLEAN);

BOOLEAN __cdecl KeSetTimerEx(PKTIMER Timer, LARGE_INTEGER DueTime, LONG Period, PKDPC Dpc)
{
    return rxdk_krnl_KeSetTimerEx(Timer, DueTime, Period, Dpc);
}

NTSTATUS __cdecl PhyInitialize(BOOLEAN forceReset, PVOID param)
{
    return rxdk_krnl_PhyInitialize(forceReset, param);
}

ULONG __cdecl PhyGetLinkState(BOOLEAN update)
{
    return rxdk_krnl_PhyGetLinkState(update);
}
