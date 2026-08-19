/*
 * MSVC printf format translation (see ms_printf.c).
 */

#ifndef RXDK_MS_PRINTF_H
#define RXDK_MS_PRINTF_H

#include <stddef.h>
#include <wchar.h>

/*
 * Rewrite an MSVC-style format into the C99 spelling picolibc implements.
 *
 * Returns the format to hand to picolibc: either fmt itself (nothing to do),
 * the caller's stack buffer, or a heap block. *heap is set to that block, if
 * any, and the caller must pass it to __rxdk_ms_format_free when done.
 */
const char *__rxdk_ms_format(const char *fmt, char *buf, size_t cap, char **heap);
const wchar_t *__rxdk_ms_wformat(const wchar_t *fmt, wchar_t *buf, size_t cap, wchar_t **heap);

void __rxdk_ms_format_free(void *heap);

/* Comfortably larger than any format string in the SDK or its samples, so the
 * heap path is effectively unused. */
#define RXDK_MS_FORMAT_STACK 256

#endif /* RXDK_MS_PRINTF_H */
