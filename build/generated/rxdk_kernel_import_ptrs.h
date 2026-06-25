#ifndef RXDK_KERNEL_IMPORT_PTRS_H
#define RXDK_KERNEL_IMPORT_PTRS_H

/*
 * Included from init.h / ntos.h when RXDK_USB_LINK is set.
 * Definitions live in build/xapi_kernel_import_ptrs.c.
 */

struct _OBJECT_TYPE;
typedef struct _OBJECT_TYPE OBJECT_TYPE, *POBJECT_TYPE;

extern POBJECT_TYPE Rxdk_p_ExEventObjectType;
extern POBJECT_TYPE Rxdk_p_ExMutantObjectType;
extern POBJECT_TYPE Rxdk_p_ExSemaphoreObjectType;
extern POBJECT_TYPE Rxdk_p_ExTimerObjectType;
extern POBJECT_TYPE Rxdk_p_PsThreadObjectType;
extern POBJECT_TYPE Rxdk_p_IoCompletionObjectType;
extern POBJECT_TYPE Rxdk_p_IoDeviceObjectType;
extern POBJECT_TYPE Rxdk_p_IoFileObjectType;
extern POBJECT_TYPE Rxdk_p_ObDirectoryObjectType;
extern POBJECT_TYPE Rxdk_p_ObSymbolicLinkObjectType;

extern const XBOX_HARDWARE_INFO *Rxdk_p_XboxHardwareInfo;

void RxdkInitKernelImportPtrs(void);

#define ExEventObjectType (Rxdk_p_ExEventObjectType)
#define ExMutantObjectType (Rxdk_p_ExMutantObjectType)
#define ExSemaphoreObjectType (Rxdk_p_ExSemaphoreObjectType)
#define ExTimerObjectType (Rxdk_p_ExTimerObjectType)
#define PsThreadObjectType (Rxdk_p_PsThreadObjectType)
#define IoCompletionObjectType (Rxdk_p_IoCompletionObjectType)
#define IoDeviceObjectType (Rxdk_p_IoDeviceObjectType)
#define IoFileObjectType (Rxdk_p_IoFileObjectType)
#define ObDirectoryObjectType (Rxdk_p_ObDirectoryObjectType)
#define ObSymbolicLinkObjectType (Rxdk_p_ObSymbolicLinkObjectType)
#define XboxHardwareInfo (Rxdk_p_XboxHardwareInfo)

#endif /* RXDK_KERNEL_IMPORT_PTRS_H */
