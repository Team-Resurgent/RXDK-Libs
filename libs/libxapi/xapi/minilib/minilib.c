#include "minilib.h"

void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d == s || n == 0) {
        return dst;
    }
    if (d < s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    unsigned char b = (unsigned char)c;
    for (size_t i = 0; i < n; i++) {
        d[i] = b;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *p = (const unsigned char *)a;
    const unsigned char *q = (const unsigned char *)b;
    for (size_t i = 0; i < n; i++) {
        if (p[i] != q[i]) {
            return (int)p[i] - (int)q[i];
        }
    }
    return 0;
}

size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++) != '\0') {
    }
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = '\0';
    }
    return dst;
}

char *strcat(char *dst, const char *src)
{
    char *d = dst + strlen(dst);
    while ((*d++ = *src++) != '\0') {
    }
    return dst;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

unsigned long strtoul(const char *s, char **end, int base)
{
    const char *p = s;
    int neg = 0;
    unsigned long acc = 0;

    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '-') {
        neg = 1;
        p++;
    } else if (*p == '+') {
        p++;
    }
    if (base == 0) {
        base = (*p == '0') ? 8 : 10;
        if (base == 8 && (p[1] == 'x' || p[1] == 'X')) {
            base = 16;
            p += 2;
        }
    }
    for (;; p++) {
        int digit;
        if (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } else if (*p >= 'a' && *p <= 'z') {
            digit = *p - 'a' + 10;
        } else if (*p >= 'A' && *p <= 'Z') {
            digit = *p - 'A' + 10;
        } else {
            break;
        }
        if (digit >= base) {
            break;
        }
        acc = acc * (unsigned long)base + (unsigned long)digit;
    }
    if (end) {
        *end = (char *)p;
    }
    return neg ? (unsigned long)(-(long)acc) : acc;
}

int __cdecl _stricmp(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = (char)((*a >= 'A' && *a <= 'Z') ? *a + 32 : *a);
        char cb = (char)((*b >= 'A' && *b <= 'Z') ? *b + 32 : *b);
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int __cdecl _wcsicmp(const wchar_t *a, const wchar_t *b)
{
    while (*a && *b) {
        wchar_t ca = (*a >= L'A' && *a <= L'Z') ? *a + 32 : *a;
        wchar_t cb = (*b >= L'A' && *b <= L'Z') ? *b + 32 : *b;
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        a++;
        b++;
    }
    return (int)*a - (int)*b;
}

static int minilib_emit_char(char **out, size_t *cap, char c)
{
    if (*cap == 0) {
        return -1;
    }
    **out = c;
    (*out)++;
    (*cap)--;
    return 0;
}

static int minilib_emit_str(char **out, size_t *cap, const char *s)
{
    while (*s) {
        if (minilib_emit_char(out, cap, *s++) != 0) {
            return -1;
        }
    }
    return 0;
}

static int minilib_emit_ulong(char **out, size_t *cap, unsigned long v, int base, int upper)
{
    char tmp[32];
    int n = 0;
    if (v == 0) {
        return minilib_emit_char(out, cap, '0');
    }
    while (v) {
        int d = (int)(v % (unsigned long)base);
        tmp[n++] = (char)(d < 10 ? '0' + d : (upper ? 'A' : 'a') + (d - 10));
        v /= (unsigned long)base;
    }
    while (n > 0) {
        if (minilib_emit_char(out, cap, tmp[--n]) != 0) {
            return -1;
        }
    }
    return 0;
}

int __cdecl vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
    char *out = buf;
    size_t cap = (n > 0) ? n - 1 : 0;
    int count = 0;

    if (!buf || n == 0) {
        return 0;
    }

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            if (cap == 0) {
                goto trunc;
            }
            *out++ = *fmt;
            cap--;
            count++;
            continue;
        }
        fmt++;
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        if (*fmt == 'l' && fmt[1] == 'u') {
            fmt += 2;
            unsigned long v = va_arg(ap, unsigned long);
            char scratch[32];
            char *p = scratch;
            size_t scap = sizeof(scratch);
            if (minilib_emit_ulong(&p, &scap, v, 10, 0) != 0) {
                goto trunc;
            }
            *p = '\0';
            for (const char *s = scratch; *s; s++) {
                if (cap == 0) {
                    goto trunc;
                }
                *out++ = *s;
                cap--;
                count++;
            }
            continue;
        }
        if (*fmt == 's') {
            const char *s = va_arg(ap, const char *);
            if (!s) {
                s = "(null)";
            }
            while (*s) {
                if (cap == 0) {
                    goto trunc;
                }
                *out++ = *s++;
                cap--;
                count++;
            }
            (void)width;
            continue;
        }
        if (*fmt == 'c') {
            char c = (char)va_arg(ap, int);
            if (cap == 0) {
                goto trunc;
            }
            *out++ = c;
            cap--;
            count++;
            continue;
        }
        if (*fmt == 'd' || *fmt == 'i') {
            long v = va_arg(ap, int);
            if (v < 0) {
                if (cap == 0) {
                    goto trunc;
                }
                *out++ = '-';
                cap--;
                count++;
                v = -v;
            }
            char scratch[32];
            char *p = scratch;
            size_t scap = sizeof(scratch);
            if (minilib_emit_ulong(&p, &scap, (unsigned long)v, 10, 0) != 0) {
                goto trunc;
            }
            *p = '\0';
            if (minilib_emit_str(&out, &cap, scratch) != 0) {
                goto trunc;
            }
            count += (int)strlen(scratch);
            continue;
        }
        if (*fmt == 'u') {
            unsigned v = va_arg(ap, unsigned);
            char scratch[32];
            char *p = scratch;
            size_t scap = sizeof(scratch);
            if (minilib_emit_ulong(&p, &scap, v, 10, 0) != 0) {
                goto trunc;
            }
            *p = '\0';
            if (minilib_emit_str(&out, &cap, scratch) != 0) {
                goto trunc;
            }
            count += (int)strlen(scratch);
            continue;
        }
        if (*fmt == 'x' || *fmt == 'X') {
            unsigned long v = va_arg(ap, unsigned long);
            char scratch[32];
            char *p = scratch;
            size_t scap = sizeof(scratch);
            if (minilib_emit_ulong(&p, &scap, v, 16, *fmt == 'X') != 0) {
                goto trunc;
            }
            *p = '\0';
            if (minilib_emit_str(&out, &cap, scratch) != 0) {
                goto trunc;
            }
            count += (int)strlen(scratch);
            continue;
        }
        if (*fmt == '%') {
            if (cap == 0) {
                goto trunc;
            }
            *out++ = '%';
            cap--;
            count++;
        }
    }
    *out = '\0';
    return count;

trunc:
    if (n > 0) {
        buf[n - 1] = '\0';
    }
    return -1;
}

int __cdecl vsprintf(char *buf, const char *fmt, va_list ap)
{
    return vsnprintf(buf, (size_t)-1 >> 1, fmt, ap);
}

int __cdecl vswprintf(wchar_t *buf, size_t n, const wchar_t *fmt, va_list ap)
{
    (void)ap;
    if (!buf || n == 0) {
        return 0;
    }
    if (fmt && fmt[0] == L'%' && fmt[1] == L's' && fmt[2] == L'\0') {
        const char *narrow = va_arg(ap, const char *);
        size_t i = 0;
        if (!narrow) {
            narrow = "(null)";
        }
        for (; narrow[i] && i + 1 < n; i++) {
            buf[i] = (wchar_t)(unsigned char)narrow[i];
        }
        buf[i] = L'\0';
        return (int)i;
    }
    buf[0] = L'\0';
    return -1;
}

int __cdecl _snprintf(char *buf, size_t n, const char *fmt, ...)
{
    va_list ap;
    int r;
    if (!buf || n == 0) {
        return 0;
    }
    va_start(ap, fmt);
    r = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}

int __cdecl _snwprintf(wchar_t *buf, size_t n, const wchar_t *fmt, ...)
{
    va_list ap;
    int r;
    if (!buf || n == 0) {
        return 0;
    }
    va_start(ap, fmt);
    r = vswprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}
