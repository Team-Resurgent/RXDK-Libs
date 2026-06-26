/*
 * <sys/random.h> for RXDK-LibsZig.
 *
 * picolibc declares getentropy() in <sys/unistd.h>, but some consumers
 * (e.g. libc++ <random>'s getentropy backend) include <sys/random.h>. This
 * provides that header. getentropy() is implemented in the RXDK libc
 * (xbox/stubs.c, kernel-backed). getrandom() is declared for source
 * compatibility; it is not currently provided.
 */
#ifndef _SYS_RANDOM_H
#define _SYS_RANDOM_H

#include <sys/cdefs.h>
#define __need_size_t
#include <stddef.h>
#include <sys/_types.h>

_BEGIN_STD_C

#ifndef __ssize_t_defined
typedef __ssize_t ssize_t;
#define __ssize_t_defined
#endif

#define GRND_NONBLOCK 0x0001
#define GRND_RANDOM   0x0002

int getentropy(void *, size_t) __picolibc_export;
ssize_t getrandom(void *, size_t, unsigned int) __picolibc_export;

_END_STD_C

#endif /* _SYS_RANDOM_H */
