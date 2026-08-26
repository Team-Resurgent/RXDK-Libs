/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Hal* kernel imports: the hardware abstraction layer. Direct access to the
 * console's low-level hardware -- the SMBus (system management bus reaching the
 * SMC, temperature sensors, and video encoder), PCI configuration space,
 * interrupt controller, the disc tray, port-I/O block moves, and the reset /
 * shutdown / return-to-firmware paths. Most titles reach this hardware through
 * higher-level services rather than calling Hal* directly.
 */

#ifndef XBOXKRNL_API_HAL_H
#define XBOXKRNL_API_HAL_H

/* Video mode the SMC reported at boot (set by the firmware, read-only). */
XBAPI DWORD HalBootSMCVideoMode;

/* Clears a pending software interrupt at the given IRQL. Fast call. */
VOID FASTCALL HalClearSoftwareInterrupt(
    IN KIRQL RequestIrql);

/* Masks (disables) the given bus interrupt level in the interrupt controller. */
VOID STDCALL HalDisableSystemInterrupt(
    IN ULONG BusInterruptLevel);

/* Number of hard-disk cache partitions configured by the firmware. */
XBAPI ULONG HalDiskCachePartitionCount;

/* ATA model-number string of the console's hard disk. */
XBAPI STRING HalDiskModelNumber;

/* ATA serial-number string of the console's hard disk. */
XBAPI STRING HalDiskSerialNumber;

/* Arms the DVD tray so the next eject is treated as a secure/user eject. */
VOID STDCALL HalEnableSecureTrayEject(void);

/* Unmasks (enables) a bus interrupt level, latching it in the given mode. */
VOID STDCALL HalEnableSystemInterrupt(
    IN ULONG BusInterruptLevel,
    IN KINTERRUPT_MODE InterruptMode);

/* Maps a bus interrupt level to its interrupt vector, returning the associated
 * IRQL in *Irql. */
ULONG STDCALL HalGetInterruptVector(
    IN ULONG BusInterruptLevel,
    OUT PKIRQL Irql);

/* Begins an orderly console shutdown. */
VOID STDCALL HalInitiateShutdown(void);

/* Returns TRUE if a reset or shutdown has been requested and is in progress. */
BOOLEAN STDCALL HalIsResetOrShutdownPending(void);

/* Reads a byte or (when ReadWordValue is TRUE) word value from an SMBus device.
 * SlaveAddress/CommandCode select the device and register; the result lands in
 * *DataValue. */
NTSTATUS STDCALL HalReadSMBusValue(
    IN UCHAR SlaveAddress,
    IN UCHAR CommandCode,
    IN BOOLEAN ReadWordValue,
    OUT ULONG* DataValue);

/* Tray-state values returned by HalReadSMCTrayState (SMC register 0x03). */
#define SMC_TRAY_STATE_CLOSED 0x00
#define SMC_TRAY_STATE_OPEN 0x10
#define SMC_TRAY_STATE_UNLOADING 0x20
#define SMC_TRAY_STATE_OPENING 0x30
#define SMC_TRAY_STATE_NO_MEDIA 0x40
#define SMC_TRAY_STATE_CLOSING 0x50
#define SMC_TRAY_STATE_MEDIA_DETECT 0x60

/* Reads the SMC tray state (one of the SMC_TRAY_STATE_* values above) into
 * *TrayState; the running change counter is returned in *TrayStateChangeCount
 * when supplied. */
NTSTATUS STDCALL HalReadSMCTrayState(
    OUT PULONG TrayState,
    OUT PULONG TrayStateChangeCount OPTIONAL);

/* Reads Length bytes of PCI configuration space (or writes them when
 * WritePCISpace is TRUE) for the device at Bus/Slot/Register into/from Buffer. */
VOID STDCALL HalReadWritePCISpace(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG RegisterNumber,
    IN PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN WritePCISpace);

/* Registers (Register TRUE) or removes (FALSE) a shutdown-notification callback
 * described by ShutdownRegistration. */
VOID STDCALL HalRegisterShutdownNotification(
    IN PHAL_SHUTDOWN_REGISTRATION ShutdownRegistration,
    IN BOOLEAN Register);

/* Requests a software interrupt at RequestIrql, to be delivered when the IRQL
 * drops. Fast call. */
VOID FASTCALL HalRequestSoftwareInterrupt(
    KIRQL RequestIrql);

/* Hands control back to the firmware, performing Routine (reboot, quick reboot,
 * or halt). Does not return. */
VOID DECLSPEC_NORETURN STDCALL HalReturnToFirmware(
    IN FIRMWARE_REENTRY Routine);

/* Writes a byte or (when WriteWordValue is TRUE) word DataValue to the SMBus
 * device/register selected by SlaveAddress/CommandCode. */
NTSTATUS STDCALL HalWriteSMBusValue(
    IN UCHAR SlaveAddress,
    IN UCHAR CommandCode,
    IN BOOLEAN WriteWordValue,
    IN ULONG DataValue);

/* Writes the SMC scratch register, used to pass a reason code across a reboot. */
NTSTATUS STDCALL HalWriteSMCScratchRegister(
    IN ULONG ScratchRegister);

/* Writes Count consecutive UCHARs from Buffer to I/O Port. */
VOID STDCALL WRITE_PORT_BUFFER_UCHAR(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count);

/* Writes Count consecutive ULONGs from Buffer to I/O Port. */
VOID STDCALL WRITE_PORT_BUFFER_ULONG(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count);

/* Writes Count consecutive USHORTs from Buffer to I/O Port. */
VOID STDCALL WRITE_PORT_BUFFER_USHORT(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count);

#endif
