/*
 * Picolibc stdin/stdout/stderr for Xbox PE/lld.
 * Vendor posixiob_*.c uses __weak_reference for stdout et al.; lld does not
 * apply those aliases, so we provide strong FILE* globals here instead.
 */
#include "local-stdio.h"

static char read_buf[BUFSIZ];
static struct __file_bufio __stdin = FDEV_SETUP_POSIX(0, read_buf, BUFSIZ, __SRD, 0);
FILE *const __posix_stdin = &__stdin.xfile.cfile.file;
FILE *const stdin = &__stdin.xfile.cfile.file;

static char write_buf[BUFSIZ];
static struct __file_bufio __stdout = FDEV_SETUP_POSIX(1, write_buf, BUFSIZ, __SWR, __BLBF);
FILE *const __posix_stdout = &__stdout.xfile.cfile.file;
FILE *const stdout = &__stdout.xfile.cfile.file;

#ifndef __PICOLIBC_STDERR_BUFSIZ
#define __PICOLIBC_STDERR_BUFSIZ 1
#endif

static char err_buf[__PICOLIBC_STDERR_BUFSIZ];
static struct __file_bufio __stderr =
    FDEV_SETUP_POSIX(2, err_buf, __PICOLIBC_STDERR_BUFSIZ, __SWR, __BLBF);
FILE *const __posix_stderr = &__stderr.xfile.cfile.file;
FILE *const stderr = &__stderr.xfile.cfile.file;

__attribute__((destructor(101))) static void posix_stdio_exit(void)
{
    if (stdout)
        fflush(stdout);
}
