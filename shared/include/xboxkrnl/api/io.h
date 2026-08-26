/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Io* kernel imports: the I/O manager and driver model. IRP allocation and
 * dispatch, device and symbolic-link creation, file open/query, volume mounting,
 * and the file-system cache (Fsc*) controls. Titles usually reach files through
 * the Nt* file APIs; the routines here are the driver-facing layer beneath them.
 */

#ifndef XBOXKRNL_API_IO_H
#define XBOXKRNL_API_IO_H

/* Returns the file-system cache size, in pages. */
PFN_COUNT STDCALL FscGetCacheSize(void);

/* Drops cache blocks that are currently idle, freeing their pages. */
VOID STDCALL FscInvalidateIdleBlocks(void);

/* Sets the file-system cache size to NumberOfCachePages pages. */
NTSTATUS STDCALL FscSetCacheSize(
    IN PFN_COUNT NumberOfCachePages);

/* The IDE channel object describing the console's ATA controller. */
XBAPI IDE_CHANNEL_OBJECT IdexChannelObject;

/* Allocates an IRP with room for StackSize stack locations. Free with
 * IoFreeIrp. */
PIRP STDCALL IoAllocateIrp(
    IN CCHAR StackSize);

/* Builds an asynchronous read/write IRP (MajorFunction) for DeviceObject over
 * Buffer/Length at StartingOffset; completion is reported via IoStatusBlock. */
PIRP STDCALL IoBuildAsynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL);

/* Builds a device-I/O-control IRP for IoControlCode with the given input/output
 * buffers; Event is signalled and IoStatusBlock filled on completion.
 * InternalDeviceIoControl selects the internal major function. */
PIRP STDCALL IoBuildDeviceIoControlRequest(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock);

/* Builds a synchronous read/write IRP; the caller waits on Event for
 * completion. */
PIRP STDCALL IoBuildSynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock);

/* Checks a new open of FileObject against existing share access; when Update is
 * TRUE the granted access is recorded in ShareAccess. */
NTSTATUS STDCALL IoCheckShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN BOOLEAN Update);

/* The object type for I/O completion ports. */
XBAPI OBJECT_TYPE IoCompletionObjectType[1];

/* Creates a device object owned by DriverObject with DeviceExtensionSize bytes
 * of driver context; returns it in *DeviceObject. */
NTSTATUS STDCALL IoCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN POBJECT_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN BOOLEAN Exclusive,
    OUT PDEVICE_OBJECT* DeviceObject);

/* Opens or creates a file, returning a handle. Disposition selects
 * create/open/overwrite behavior; CreateOptions/Options carry FILE_* flags.
 * Result detail is returned in IoStatusBlock. */
NTSTATUS STDCALL IoCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN ULONG Options);

/* Creates a namespace symbolic link SymbolicLinkName pointing at DeviceName. */
NTSTATUS STDCALL IoCreateSymbolicLink(
    IN POBJECT_STRING SymbolicLinkName,
    IN POBJECT_STRING DeviceName);

/* Deletes a device object created by IoCreateDevice. */
VOID STDCALL IoDeleteDevice(
    IN PDEVICE_OBJECT DeviceObject);

/* Removes a symbolic link created by IoCreateSymbolicLink. */
NTSTATUS STDCALL IoDeleteSymbolicLink(
    IN POBJECT_STRING SymbolicLinkName);

/* The object type for device objects. */
XBAPI OBJECT_TYPE IoDeviceObjectType[1];

/* Dismounts the volume mounted on DeviceObject. */
NTSTATUS STDCALL IoDismountVolume(
    IN PDEVICE_OBJECT DeviceObject);

/* Dismounts the volume mounted on the named device. */
NTSTATUS STDCALL IoDismountVolumeByName(
    IN POBJECT_STRING DeviceName);

/* Passes Irp to the next-lower driver for DeviceObject. Returns that driver's
 * status. Fast call; the core IRP dispatch primitive. */
NTSTATUS FASTCALL IofCallDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp);

/* Completes Irp, running its completion routines and, optionally, boosting the
 * waiting thread's priority by PriorityBoost. Fast call. */
VOID FASTCALL IofCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost);

/* The object type for file objects. */
XBAPI OBJECT_TYPE IoFileObjectType[1];

/* Frees an IRP from IoAllocateIrp. */
VOID STDCALL IoFreeIrp(
    IN PIRP Irp);

/* Initializes a caller-supplied IRP buffer of PacketSize bytes with StackSize
 * stack locations. */
VOID STDCALL IoInitializeIrp(
    IN OUT PIRP Irp,
    IN USHORT PacketSize,
    IN CCHAR StackSize);

/* Default dispatch routine that fails an IRP with an invalid-request status. */
NTSTATUS STDCALL IoInvalidDeviceRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

/* Marks Irp so it cannot be cancelled and must run to completion. */
VOID STDCALL IoMarkIrpMustComplete(
    IN OUT PIRP Irp);

/* Queries FileInformationClass metadata for an open file into FileInformation
 * (up to Length bytes); bytes written are returned in *ReturnedLength. */
NTSTATUS STDCALL IoQueryFileInformation(
    IN PFILE_OBJECT FileObject,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG Length,
    OUT PVOID FileInformation,
    OUT PULONG ReturnedLength);

/* Queries FsInformationClass metadata for the file's volume into FsInformation. */
NTSTATUS STDCALL IoQueryVolumeInformation(
    IN PFILE_OBJECT FileObject,
    IN FS_INFORMATION_CLASS FsInformationClass,
    IN ULONG Length,
    OUT PVOID FsInformation,
    OUT PULONG ReturnedLength);

/* Queues Irp onto the current thread's pending-IRP list. */
VOID STDCALL IoQueueThreadIrp(
    IN PIRP Irp);

/* Releases the share access this file object held, updating ShareAccess. */
VOID STDCALL IoRemoveShareAccess(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess);

/* Posts a completion packet (KeyContext/ApcContext, IoStatus,
 * IoStatusInformation) to an I/O completion object. */
NTSTATUS STDCALL IoSetIoCompletion(
    IN PVOID IoCompletion,
    IN PVOID KeyContext,
    IN PVOID ApcContext,
    IN NTSTATUS IoStatus,
    IN ULONG_PTR IoStatusInformation);

/* Records the initial share access for a newly opened FileObject. */
VOID STDCALL IoSetShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    OUT PSHARE_ACCESS ShareAccess);

/* Dequeues and starts the next IRP on DeviceObject's serialized queue. */
VOID STDCALL IoStartNextPacket(
    IN PDEVICE_OBJECT DeviceObject);

/* As IoStartNextPacket, honoring the by-key ordering with the given Key. */
VOID STDCALL IoStartNextPacketByKey(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Key);

/* Serializes Irp onto DeviceObject, starting it now if the device is idle;
 * Key orders it within the queue. */
VOID STDCALL IoStartPacket(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PULONG Key OPTIONAL);

/* Issues a device I/O control synchronously, blocking until it completes; bytes
 * returned are reported in *ReturnedOutputBufferLength. */
NTSTATUS STDCALL IoSynchronousDeviceIoControlRequest(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG ReturnedOutputBufferLength OPTIONAL,
    IN BOOLEAN InternalDeviceIoControl);

/* Issues a synchronous read/write (MajorFunction) to DeviceObject, blocking
 * until it completes. */
NTSTATUS STDCALL IoSynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL);

#endif
