#include <stdarg.h>
#include <stdio.h>

int __i_vfprintf(FILE *stream, const char *format, va_list ap);
int __i_vfscanf(FILE *stream, const char *format, va_list ap);

int vfprintf(FILE *stream, const char *format, va_list ap)
{
    return __i_vfprintf(stream, format, ap);
}

int vfscanf(FILE *stream, const char *format, va_list ap)
{
    return __i_vfscanf(stream, format, ap);
}
