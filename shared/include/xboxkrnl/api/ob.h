/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Ob* kernel imports: the object manager. Kernel objects (events, mutexes,
 * files, directories, symbolic links) are reference-counted bodies reached by
 * name or handle. These routines create objects, translate between handles,
 * names, and pointers, and adjust reference counts. The Obf* fast-call pair is
 * the hot reference/dereference path.
 */

#ifndef XBOXKRNL_API_OB_H
#define XBOXKRNL_API_OB_H

/* Allocates an object body of ObjectBodySize bytes of the given ObjectType,
 * returning it in *Object. The caller initializes and then inserts the body. */
NTSTATUS STDCALL ObCreateObject(
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG ObjectBodySize,
    OUT PVOID* Object);

/* The object type for namespace directories. */
XBAPI OBJECT_TYPE ObDirectoryObjectType[1];

/* Drops one reference on Object, freeing it when the count reaches zero. Fast
 * call. */
VOID FASTCALL ObfDereferenceObject(
    IN PVOID Object);

/* Adds one reference to Object. Fast call. */
VOID FASTCALL ObfReferenceObject(
    IN PVOID Object);

/* Inserts a newly created object into the namespace/handle table and returns a
 * handle. ObjectPointerBias adds extra references retained by the caller. */
NTSTATUS STDCALL ObInsertObject(
    IN PVOID Object,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG ObjectPointerBias,
    OUT PHANDLE Handle);

/* Marks Object temporary so it is deleted once its last handle/reference goes
 * away rather than persisting in the namespace. */
VOID STDCALL ObMakeTemporaryObject(
    IN PVOID Object);

/* Opens a named object, returning a handle. ObjectType must match the object;
 * ParseContext is passed to the type's parse routine. */
NTSTATUS STDCALL ObOpenObjectByName(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PHANDLE Handle);

/* Creates a new handle referring to an already-referenced object pointer. */
NTSTATUS STDCALL ObOpenObjectByPointer(
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType,
    OUT PHANDLE Handle);

/* The process-wide handle-to-object table. */
XBAPI OBJECT_HANDLE_TABLE ObpObjectHandleTable;

/* Validates Handle, optionally checks its type, and returns a referenced object
 * pointer in *ReturnedObject. Balance with ObfDereferenceObject. */
NTSTATUS STDCALL ObReferenceObjectByHandle(
    IN HANDLE Handle,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    OUT PVOID* ReturnedObject);

/* Looks up an object by name and returns a referenced pointer in *Object. */
NTSTATUS STDCALL ObReferenceObjectByName(
    IN POBJECT_STRING ObjectName,
    IN ULONG Attributes,
    IN POBJECT_TYPE ObjectType,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID* Object);

/* Adds a reference to an object given its pointer, checking its type. */
NTSTATUS STDCALL ObReferenceObjectByPointer(
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType);

/* The object type for symbolic links in the namespace. */
XBAPI OBJECT_TYPE ObSymbolicLinkObjectType[1];

#endif
