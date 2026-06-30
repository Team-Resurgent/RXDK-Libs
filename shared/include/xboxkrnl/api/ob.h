#ifndef XBOXKRNL_API_OB_H
#define XBOXKRNL_API_OB_H

XBAPI NTSTATUS STDCALL ObCreateObject
(
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG ObjectBodySize,
    OUT PVOID *Object
);

XBAPI OBJECT_TYPE ObDirectoryObjectType[1];

XBAPI VOID FASTCALL ObfDereferenceObject
(
    IN PVOID Object
);

XBAPI VOID FASTCALL ObfReferenceObject
(
    IN PVOID Object
);

XBAPI NTSTATUS STDCALL ObInsertObject
(
    IN PVOID Object,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG ObjectPointerBias,
    OUT PHANDLE Handle
);

XBAPI VOID STDCALL ObMakeTemporaryObject
(
    IN PVOID Object
);

XBAPI NTSTATUS STDCALL ObOpenObjectByName
(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PHANDLE Handle
);

XBAPI NTSTATUS STDCALL ObOpenObjectByPointer
(
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType,
    OUT PHANDLE Handle
);

XBAPI OBJECT_HANDLE_TABLE ObpObjectHandleTable;

XBAPI NTSTATUS STDCALL ObReferenceObjectByHandle
(
    IN HANDLE Handle,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    OUT PVOID *ReturnedObject
);

XBAPI NTSTATUS STDCALL ObReferenceObjectByName
(
    IN POBJECT_STRING ObjectName,
    IN ULONG Attributes,
    IN POBJECT_TYPE ObjectType,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID *Object
);

XBAPI NTSTATUS STDCALL ObReferenceObjectByPointer
(
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType
);

XBAPI OBJECT_TYPE ObSymbolicLinkObjectType[1];

#endif
