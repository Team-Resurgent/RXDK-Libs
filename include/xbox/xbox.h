#ifndef RXDK_XBOX_H
#define RXDK_XBOX_H

#define _XBOX 1
#define _WIN32 1
#define _X86_ 1

#include "kernel.h"

void xbox_runtime_init(void);
void xbox_zero_uninitialized_data(void);
void xbox_halt(void) __attribute__((noreturn));
void xbox_trace(const char *stage);
void xbox_trace_ptr(const char *label, const void *ptr);
void xbox_trace_enter_main(void);

#endif /* RXDK_XBOX_H */
