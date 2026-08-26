/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * I/O manager driver-model structures: the FILE_OBJECT (an open handle's state),
 * the IRP (I/O request packet) with its flag bits, the DRIVER_OBJECT dispatch
 * table and its routine pointer types, and the DEVICE_OBJECT. These back the Io*
 * kernel routines and are mainly of interest to code implementing or driving a
 * device.
 */

#ifndef XBOXKRNL_TYPES_IO_H
#define XBOXKRNL_TYPES_IO_H

struct _DEVICE_OBJECT;

/* Kernel state for one open file/device instance: access and share rights, the
 * owning device, file-system context pointers, and the current byte offset. */
typedef struct _FILE_OBJECT {
    CSHORT Type;
    BOOLEAN DeletePending : 1;
    BOOLEAN ReadAccess : 1;
    BOOLEAN WriteAccess : 1;
    BOOLEAN DeleteAccess : 1;
    BOOLEAN SharedRead : 1;
    BOOLEAN SharedWrite : 1;
    BOOLEAN SharedDelete : 1;
    BOOLEAN Reserved : 1;
    UCHAR Flags;
    struct _DEVICE_OBJECT* DeviceObject;
    PVOID FsContext;
    PVOID FsContext2;
    NTSTATUS FinalStatus;
    LARGE_INTEGER CurrentByteOffset;
    struct _FILE_OBJECT* RelatedFileObject;
    PIO_COMPLETION_CONTEXT CompletionContext;
    LONG LockCount;
    KEVENT Lock;
    KEVENT Event;
} FILE_OBJECT, *PFILE_OBJECT;

/* I/O request packet: the unit of work passed down the driver stack. Carries the
 * completion status, the per-driver stack locations, user buffer/APC/event, and
 * cancellation and threading bookkeeping. Allocate with IoAllocateIrp. */
typedef struct _IRP {
    CSHORT Type;
    USHORT Size;
    ULONG Flags;
    LIST_ENTRY ThreadListEntry;
    IO_STATUS_BLOCK IoStatus;
    CHAR StackCount;
    CHAR CurrentLocation;
    BOOLEAN PendingReturned;
    BOOLEAN Cancel;
    PIO_STATUS_BLOCK UserIosb;
    PKEVENT UserEvent;

    union {
        struct
        {
            PIO_APC_ROUTINE UserApcRoutine;
            PVOID UserApcContext;
        } AsynchronousParameters;
        LARGE_INTEGER AllocationSize;
    } Overlay;

    PVOID UserBuffer;
    PFILE_SEGMENT_ELEMENT SegmentArray;
    ULONG LockedBufferLength;

    union {
        struct
        {
            union {
                KDEVICE_QUEUE_ENTRY DeviceQueueEntry;
                struct
                {
                    PVOID DriverContext[5];
                };
            };
            PETHREAD Thread;

            struct
            {
                LIST_ENTRY ListEntry;
                union {
                    struct _IO_STATUS_LOCATION* CurrentStackLocation;
                    ULONG PacketType;
                };
            };

            PFILE_OBJECT OriginalFileObject;
        } Overlay;

        KAPC Apc;
        PVOID CompletionKey;
    } Tail;
} IRP, *PIRP;

/* IRP.Flags bits recording the request kind (read/write/create/close), caching
 * and completion-deferral hints, and buffer/segment cleanup responsibilities. */
#define IRP_NOCACHE 0x00000001
#define IRP_MOUNT_COMPLETION 0x00000002
#define IRP_SYNCHRONOUS_API 0x00000004
#define IRP_CREATE_OPERATION 0x00000008
#define IRP_READ_OPERATION 0x00000010
#define IRP_WRITE_OPERATION 0x00000020
#define IRP_CLOSE_OPERATION 0x00000040
#define IRP_DEFER_IO_COMPLETION 0x00000080
#define IRP_OB_QUERY_NAME 0x00000100
#define IRP_UNLOCK_USER_BUFFER 0x00000200
#define IRP_SCATTER_GATHER_OPERATION 0x00000400
#define IRP_UNMAP_SEGMENT_ARRAY 0x00000800
#define IRP_NO_CANCELIO 0x00001000

struct _DEVICE_OBJECT;

/* Driver entry points stored in DRIVER_OBJECT: start-I/O for a serialized
 * request, device deletion, volume dismount, and the per-major-function IRP
 * dispatch handler. */
typedef VOID(STDCALL* PDRIVER_STARTIO)(

    IN struct _DEVICE_OBJECT* DeviceObject,
    IN struct _IRP* Irp);

typedef VOID(STDCALL* PDRIVER_DELETEDEVICE)(

    IN struct _DEVICE_OBJECT* DeviceObject);

typedef NTSTATUS(STDCALL* PDRIVER_DISMOUNTVOLUME)(

    IN struct _DEVICE_OBJECT* DeviceObject);

typedef NTSTATUS(STDCALL* PDRIVER_DISPATCH)(

    IN struct _DEVICE_OBJECT* DeviceObject,
    IN struct _IRP* Irp);

/* A driver's routine table: start-I/O, delete-device, dismount-volume, and the
 * MajorFunction[] dispatch array indexed by IRP major function code. */
typedef struct _DRIVER_OBJECT {
    PDRIVER_STARTIO DriverStartIo;
    PDRIVER_DELETEDEVICE DriverDeleteDevice;
    PDRIVER_DISMOUNTVOLUME DriverDismountVolume;
    PDRIVER_DISPATCH MajorFunction[0x0E];
} DRIVER_OBJECT, *PDRIVER_OBJECT;

/* Represents a device instance: its owning driver, mount relationship, the
 * per-driver DeviceExtension, the serialized request queue, and geometry
 * (SectorSize, AlignmentRequirement, StackSize). Created by IoCreateDevice. */
typedef struct _DEVICE_OBJECT {
    CSHORT Type;
    USHORT Size;
    LONG ReferenceCount;
    PDRIVER_OBJECT DriverObject;
    struct _DEVICE_OBJECT* MountedOrSelfDevice;
    PIRP CurrentIrp;
    ULONG Flags;
    PVOID DeviceExtension;
    UCHAR DeviceType;
    UCHAR StartIoFlags;
    CCHAR StackSize;
    BOOLEAN DeletePending;
    ULONG SectorSize;
    ULONG AlignmentRequirement;
    KDEVICE_QUEUE DeviceQueue;
    KEVENT DeviceLock;
    ULONG StartIoKey;
} DEVICE_OBJECT, *PDEVICE_OBJECT;

#endif
