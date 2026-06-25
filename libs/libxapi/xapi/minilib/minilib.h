#ifndef RXDK_XAPI_MINILIB_H
#define RXDK_XAPI_MINILIB_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);

size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
char *strcat(char *dst, const char *src);
int strcmp(const char *a, const char *b);
unsigned long strtoul(const char *s, char **end, int base);

int __cdecl _stricmp(const char *a, const char *b);
int __cdecl _wcsicmp(const wchar_t *a, const wchar_t *b);

int __cdecl vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
int __cdecl vsprintf(char *buf, const char *fmt, va_list ap);
int __cdecl vswprintf(wchar_t *buf, size_t n, const wchar_t *fmt, va_list ap);
int __cdecl _snprintf(char *buf, size_t n, const char *fmt, ...);
int __cdecl _snwprintf(wchar_t *buf, size_t n, const wchar_t *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* RXDK_XAPI_MINILIB_H */
