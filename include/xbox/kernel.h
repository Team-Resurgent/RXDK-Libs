#ifndef RXDK_XBOX_KERNEL_H
#define RXDK_XBOX_KERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Kernel export from prebuilt/xboxkrnl.lib (cdecl, import by ordinal). */
void DbgPrint(const char *fmt, ...);

/* Implemented in libxboxc (src/xbox/kernel.c) — forwards to DbgPrint. */
void OutputDebugStringA(const char *lpOutputString);

#ifdef __cplusplus
}
#endif

#endif /* RXDK_XBOX_KERNEL_H */
