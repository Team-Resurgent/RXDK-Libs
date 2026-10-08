/* Generated for RXDK-Libs Xbox HAL (picolibc 1.8.x profile). */
#pragma once

#define __ATOMIC_UNGETC 1
#define __FAST_STRCMP 1
#define __PICOLIBC_ERRNO_FUNCTION __rxdk_errno
#define __TINY_STDIO 1
#define __HAVE_COMPLEX 1
#define __IO_C99_FORMATS 1
#define __IO_LONG_LONG 1
/* %ls/%lc in the narrow printf family, which is what MSVC's %S/%C are rewritten
   to (libs/libc/xbox/ms_printf.c). Deliberately not __MB_CAPABLE: without it
   picolibc narrows a wchar_t by truncation rather than through a locale's
   multibyte encoding, which is the behaviour Xbox paths and gamertags want. */
#define __IO_WCHAR 1
/* Default printf/scanf I/O variant = double ('d'): float formatting/parsing for
   printf, scanf, and iostream num_put/num_get. Was 0 (no float -> picolibc
   emitted the "*float*" placeholder for %f/%g). */
#define __IO_DEFAULT 'd'
/* Exact binary <-> decimal conversion: the Ryu sources (dtoa_ryu.c etc.) are
   what build/sdk/libc.json compiles, not the approximate *_engine.c ones. */
#define __IO_FLOAT_EXACT 1
/* printf produces every requested significant digit of a double exactly
   (dtoa_big.c), as glibc and MSVC do, rather than zero-filling past 17. */
#define __IO_FLOAT_ALL_DIGITS 1
#define __NEWLIB_VERSION "4.3.0"
#define _NEWLIB_VERSION "4.3.0"
#define _PICOLIBC_MINOR__ 8
#define _PICOLIBC_VERSION "1.8.11"
#define _PICOLIBC__ 1
#define __NEWLIB_MINOR__ 3
#define __NEWLIB_PATCHLEVEL__ 0
#define __NEWLIB__ 4
#define __PICOLIBC_MINOR__ 8
#define __PICOLIBC_PATCHLEVEL__ 11
#define __PICOLIBC_VERSION__ "1.8.11"
#define __PICOLIBC__ 1
#define ENABLE_PICOLIBC_EXIT 1
#define __HAVE_POSIX_LOCALE_API 1
#define __OBSOLETE_MATH_FLOAT 0
#define __OBSOLETE_MATH_DOUBLE 0

/* long double is the 64-bit double type on the original Xbox, as with the XDK's
   MSVC compiler and on the Xbox 360: libc, libcpp and every SDK library are
   built with -mlong-double-64, and an object built with clang's 80-bit
   i686-pc-windows-gnu default would disagree with them on its size and on the
   long double math and printf entry points. The RXDK build passes the flag;
   other build systems must too. */
#if defined(__SIZEOF_LONG_DOUBLE__) && __SIZEOF_LONG_DOUBLE__ != 8
#error "RXDK: compile with -mlong-double-64 (long double is the 64-bit double type on the original Xbox)"
#endif
