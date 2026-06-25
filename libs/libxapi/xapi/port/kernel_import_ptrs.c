#include "bridge_k32.h"
/*
 * Kernel DATA imports are struct slots in the XBE IAT. Vendor NTOS headers
 * (ntos.h, !_NTSYSTEM_) treat them as POBJECT_TYPE pointer variables.
 * Initialize real pointer-to-slot values once before USB init.
 */
#include <xboxkrnl/xboxkrnl.h>

POBJECT_TYPE Rxdk_p_ExEventObjectType;
POBJECT_TYPE Rxdk_p_ExMutantObjectType;
POBJECT_TYPE Rxdk_p_ExSemaphoreObjectType;
POBJECT_TYPE Rxdk_p_ExTimerObjectType;
POBJECT_TYPE Rxdk_p_PsThreadObjectType;
POBJECT_TYPE Rxdk_p_IoCompletionObjectType;
POBJECT_TYPE Rxdk_p_IoDeviceObjectType;
POBJECT_TYPE Rxdk_p_IoFileObjectType;
POBJECT_TYPE Rxdk_p_ObDirectoryObjectType;
POBJECT_TYPE Rxdk_p_ObSymbolicLinkObjectType;
const XBOX_HARDWARE_INFO *Rxdk_p_XboxHardwareInfo;

void RxdkInitKernelImportPtrs(void)
{
    Rxdk_p_ExEventObjectType = (POBJECT_TYPE)(void *)&ExEventObjectType;
    Rxdk_p_ExMutantObjectType = (POBJECT_TYPE)(void *)&ExMutantObjectType;
    Rxdk_p_ExSemaphoreObjectType = (POBJECT_TYPE)(void *)&ExSemaphoreObjectType;
    Rxdk_p_ExTimerObjectType = (POBJECT_TYPE)(void *)&ExTimerObjectType;
    Rxdk_p_PsThreadObjectType = (POBJECT_TYPE)(void *)&PsThreadObjectType;
    Rxdk_p_IoCompletionObjectType = (POBJECT_TYPE)(void *)&IoCompletionObjectType;
    Rxdk_p_IoDeviceObjectType = (POBJECT_TYPE)(void *)&IoDeviceObjectType;
    Rxdk_p_IoFileObjectType = (POBJECT_TYPE)(void *)&IoFileObjectType;
    Rxdk_p_ObDirectoryObjectType = (POBJECT_TYPE)(void *)&ObDirectoryObjectType;
    Rxdk_p_ObSymbolicLinkObjectType = (POBJECT_TYPE)(void *)&ObSymbolicLinkObjectType;
    Rxdk_p_XboxHardwareInfo = &XboxHardwareInfo;
}
