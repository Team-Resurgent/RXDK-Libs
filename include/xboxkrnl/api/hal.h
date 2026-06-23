#ifndef XBOXKRNL_API_HAL_H
#define XBOXKRNL_API_HAL_H

XBAPI DWORD HalBootSMCVideoMode;

XBAPI VOID FASTCALL HalClearSoftwareInterrupt
(
    IN KIRQL RequestIrql
);

XBAPI VOID NTAPI HalDisableSystemInterrupt
(
    IN ULONG BusInterruptLevel
);

XBAPI ULONG HalDiskCachePartitionCount;

XBAPI STRING HalDiskModelNumber;

XBAPI STRING HalDiskSerialNumber;

XBAPI VOID NTAPI HalEnableSecureTrayEject(void);

XBAPI VOID NTAPI HalEnableSystemInterrupt
(
    IN ULONG BusInterruptLevel,
    IN KINTERRUPT_MODE InterruptMode
);

XBAPI ULONG NTAPI HalGetInterruptVector
(
    IN ULONG BusInterruptLevel,
    OUT PKIRQL Irql
);

XBAPI VOID NTAPI HalInitiateShutdown(void);

XBAPI BOOLEAN NTAPI HalIsResetOrShutdownPending(void);

XBAPI NTSTATUS NTAPI HalReadSMBusValue
(
    IN UCHAR SlaveAddress,
    IN UCHAR CommandCode,
    IN BOOLEAN ReadWordValue,
    OUT ULONG *DataValue
);

XBAPI NTSTATUS NTAPI HalReadSMCTrayState
(
    OUT PULONG TrayState,
    OUT PULONG TrayStateChangeCount OPTIONAL
);

XBAPI VOID NTAPI HalReadWritePCISpace
(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG RegisterNumber,
    IN PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN WritePCISpace
);

XBAPI VOID NTAPI HalRegisterShutdownNotification
(
    IN PHAL_SHUTDOWN_REGISTRATION ShutdownRegistration,
    IN BOOLEAN Register
);

XBAPI VOID FASTCALL HalRequestSoftwareInterrupt
(
    KIRQL RequestIrql
);

XBAPI VOID DECLSPEC_NORETURN NTAPI HalReturnToFirmware
(
    IN FIRMWARE_REENTRY Routine
);

XBAPI NTSTATUS NTAPI HalWriteSMBusValue
(
    IN UCHAR SlaveAddress,
    IN UCHAR CommandCode,
    IN BOOLEAN WriteWordValue,
    IN ULONG DataValue
);

XBAPI NTSTATUS NTAPI HalWriteSMCScratchRegister
(
    IN ULONG ScratchRegister
);

XBAPI VOID NTAPI WRITE_PORT_BUFFER_UCHAR
(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count
);

XBAPI VOID NTAPI WRITE_PORT_BUFFER_ULONG
(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
);

XBAPI VOID NTAPI WRITE_PORT_BUFFER_USHORT
(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
);

#endif
