#ifndef RXDK_XBOX_H
#define RXDK_XBOX_H

#define _XBOX 1
#define _WIN32 1
#define _X86_ 1

void xbox_runtime_init(void);
void xbox_halt(void) __attribute__((noreturn));

#endif /* RXDK_XBOX_H */
