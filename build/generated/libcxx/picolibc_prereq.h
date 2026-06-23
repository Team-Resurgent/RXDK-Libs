/* Picolibc + POSIX locale visibility for LLVM libc++ (newlib backend). */
#pragma once

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE 1
#define __HAVE_POSIX_LOCALE_API 1

#include "picolibc.h"

extern "C" {
#include <locale.h>
#include <stdlib.h>
}
