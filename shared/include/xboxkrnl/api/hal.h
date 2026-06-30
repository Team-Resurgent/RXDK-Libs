#ifndef XBOXKRNL_API_HAL_H
#define XBOXKRNL_API_HAL_H

XBAPI DWORD HalBootSMCVideoMode;

VOID FASTCALL HalClearSoftwareInterrupt
(
    IN KIRQL RequestIrql
);

VOID STDCALL HalDisableSystemInterrupt
(
    IN ULONG BusInterruptLevel
);

XBAPI ULONG HalDiskCachePartitionCount;

XBAPI STRING HalDiskModelNumber;

XBAPI STRING HalDiskSerialNumber;

VOID STDCALL HalEnableSecureTrayEject(void);

VOID STDCALL HalEnableSystemInterrupt
(
    IN ULONG BusInterruptLevel,
    IN KINTERRUPT_MODE InterruptMode
);

ULONG STDCALL HalGetInterruptVector
(
    IN ULONG BusInterruptLevel,
    OUT PKIRQL Irql
);

VOID STDCALL HalInitiateShutdown(void);

BOOLEAN STDCALL HalIsResetOrShutdownPending(void);

NTSTATUS STDCALL HalReadSMBusValue
(
    IN UCHAR SlaveAddress,
    IN UCHAR CommandCode,
    IN BOOLEAN ReadWordValue,
    OUT ULONG *DataValue
);

NTSTATUS STDCALL HalReadSMCTrayState
(
    OUT PULONG TrayState,
    OUT PULONG TrayStateChangeCount OPTIONAL
);

VOID STDCALL HalReadWritePCISpace
(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG RegisterNumber,
    IN PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN WritePCISpace
);

VOID STDCALL HalRegisterShutdownNotification
(
    IN PHAL_SHUTDOWN_REGISTRATION ShutdownRegistration,
    IN BOOLEAN Register
);

VOID FASTCALL HalRequestSoftwareInterrupt
(
    KIRQL RequestIrql
);

VOID DECLSPEC_NORETURN STDCALL HalReturnToFirmware
(
    IN FIRMWARE_REENTRY Routine
);

NTSTATUS STDCALL HalWriteSMBusValue
(
    IN UCHAR SlaveAddress,
    IN UCHAR CommandCode,
    IN BOOLEAN WriteWordValue,
    IN ULONG DataValue
);

NTSTATUS STDCALL HalWriteSMCScratchRegister
(
    IN ULONG ScratchRegister
);

VOID STDCALL WRITE_PORT_BUFFER_UCHAR
(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count
);

VOID STDCALL WRITE_PORT_BUFFER_ULONG
(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
);

VOID STDCALL WRITE_PORT_BUFFER_USHORT
(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
);

#endif
