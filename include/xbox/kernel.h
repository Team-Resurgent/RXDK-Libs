#ifndef RXDK_XBOX_KERNEL_H
#define RXDK_XBOX_KERNEL_H

/*
 * Full xboxkrnl import surface (clean headers under include/xboxkrnl/).
 * Link prebuilt/xboxkrnl.lib — only referenced imports are pulled in.
 */
#include <xboxkrnl/xboxkrnl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HAL helper in src/xbox/kernel.c — forwards to DbgPrint. */
void OutputDebugStringA(const char *lpOutputString);

#ifdef __cplusplus
}
#endif

#endif /* RXDK_XBOX_KERNEL_H */
