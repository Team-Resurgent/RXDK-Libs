#include "common.h"

#include <stdlib.h>

//
// MSVC _snprintf/_vsnprintf semantics.
//
// The size argument does NOT mean what it means in C99. MSVC writes at most
// `count` characters and only NUL-terminates when the result fits in fewer than
// count; C99 writes at most size-1 characters and always terminates. Aliasing
// one to the other silently truncates by a character, which is exactly the bug
// this test exists to prevent regressing.
//

// Declared here rather than pulled in from xdk_compat.h: that header ships to
// titles from dist-include, which this in-tree sample does not compile against.
// What matters for the regression is the runtime behaviour of the libc
// implementation (libs/libc/xbox/msvc_printf.c), which these prototypes bind to.
int _snprintf(char *buffer, size_t count, const char *format, ...);
int _vsnprintf(char *buffer, size_t count, const char *format, va_list ap);
int _scprintf(const char *format, ...);
int _vscprintf(const char *format, va_list ap);

static int check_vsnprintf(char *buffer, size_t count, const char *format, ...)
{
    va_list ap;
    int     result;

    va_start(ap, format);
    result = _vsnprintf(buffer, count, format, ap);
    va_end(ap);

    return result;
}

int test_msvc_printf(void)
{
    char buffer[16];
    int  result;

    /* Measuring: NULL/0 reports the length the result would need. */
    if (_scprintf("%i", 12345) != 5) {
        return 1;
    }

    if (check_vsnprintf(NULL, 0, "%i", 12345) != 5) {
        return 2;
    }

    /* MSVC idiom: count == length writes every character and does NOT
       terminate, returning -1. This is the case the old alias broke -- it used
       to drop the trailing '5'. */
    memset(buffer, 'x', sizeof(buffer));
    result = check_vsnprintf(buffer, 5, "%i", 12345);

    if (result != -1) {
        return 3;
    }

    if (memcmp(buffer, "12345", 5) != 0) {
        return 4; /* last character lost */
    }

    if (buffer[5] != 'x') {
        return 5; /* wrote past count */
    }

    /* C99-style call: count == length + 1 leaves room for the terminator, so
       the whole string plus NUL lands and the length is returned. */
    memset(buffer, 'x', sizeof(buffer));
    result = check_vsnprintf(buffer, 6, "%i", 12345);

    if (result != 5) {
        return 6;
    }

    if (strcmp(buffer, "12345") != 0) {
        return 7;
    }

    /* Genuine truncation: fill count exactly, no terminator, report -1. */
    memset(buffer, 'x', sizeof(buffer));
    result = check_vsnprintf(buffer, 3, "%s", "abcdefgh");

    if (result != -1) {
        return 8;
    }

    if (memcmp(buffer, "abc", 3) != 0) {
        return 9;
    }

    if (buffer[3] != 'x') {
        return 10;
    }

    /* The varargs spelling must agree with the va_list one. */
    memset(buffer, 'x', sizeof(buffer));

    if (_snprintf(buffer, 6, "%i", 12345) != 5 || strcmp(buffer, "12345") != 0) {
        return 11;
    }

    /* Round-trip the idiom ported titles actually use. */
    {
        int   length = _scprintf("%s-%i", "key", 65);
        char *text   = (char *)malloc(length + 1);

        if (text == NULL) {
            return 12;
        }

        _snprintf(text, length, "%s-%i", "key", 65);
        text[length] = 0;

        if (strcmp(text, "key-65") != 0) {
            free(text);
            return 13;
        }

        free(text);
    }

    return XAPI_OK;
}
