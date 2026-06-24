#!/usr/bin/env python3
"""Runtime conformance test bodies for RXDK kit libc/C23 probes.

Header taxonomy follows vendor/stdtests/template/c_header.txt.
Add a (group, name, body) tuple here; regenerate with generate_conformance_tests.py.
Bodies must not #include headers (generator adds file-scope includes).
"""

from __future__ import annotations

CONFORMANCE_TESTS: list[tuple[str, str, str]] = [
    (
        "string",
        "strlen",
        """
    RXDK_TEST_EQ(strlen(""), 0u);
    RXDK_TEST_EQ(strlen("abc"), 3u);
    return 0;
""",
    ),
    (
        "string",
        "strcmp",
        """
    RXDK_TEST_EQ(strcmp("abc", "abc"), 0);
    RXDK_TEST_TRUE(strcmp("abc", "abd") < 0);
    RXDK_TEST_TRUE(strcmp("abd", "abc") > 0);
    return 0;
""",
    ),
    (
        "string",
        "memcpy",
        """
    char dst[8];
    RXDK_TEST_EQ(memcpy(dst, "xy", 3), dst);
    RXDK_TEST_STR_EQ(dst, "xy");
    return 0;
""",
    ),
    (
        "string",
        "memset",
        """
    unsigned char buf[4];
    memset(buf, 0xA5, sizeof buf);
    for (unsigned i = 0; i < sizeof buf; i++)
        if (buf[i] != 0xA5)
            RXDK_TEST_FAIL();
    return 0;
""",
    ),
    (
        "string",
        "strchr",
        """
    const char *p = strchr("abc", 'b');
    RXDK_TEST_TRUE(p != NULL && p[0] == 'b' && p[1] == 'c');
    RXDK_TEST_EQ(strchr("abc", 'z'), NULL);
    return 0;
""",
    ),
    (
        "stdlib",
        "atoi",
        """
    RXDK_TEST_EQ(atoi("42"), 42);
    RXDK_TEST_EQ(atoi("-7"), -7);
    return 0;
""",
    ),
    (
        "stdlib",
        "strtol",
        """
    char *end = NULL;
    long v = strtol("0x10", &end, 16);
    RXDK_TEST_EQ(v, 16L);
    RXDK_TEST_TRUE(end != NULL && *end == '\\0');
    return 0;
""",
    ),
    (
        "stdlib",
        "malloc_free",
        """
    void *p = malloc(64);
    if (p == NULL)
        RXDK_TEST_FAIL();
    free(p);
    return 0;
""",
    ),
    (
        "ctype",
        "isdigit",
        """
    RXDK_TEST_TRUE(isdigit('0'));
    RXDK_TEST_TRUE(isdigit('9'));
    RXDK_TEST_FALSE(isdigit('a'));
    return 0;
""",
    ),
    (
        "ctype",
        "toupper",
        """
    RXDK_TEST_EQ(toupper('a'), 'A');
    RXDK_TEST_EQ(toupper('Z'), 'Z');
    return 0;
""",
    ),
    (
        "errno",
        "errno_rw",
        """
    errno = 123;
    RXDK_TEST_EQ(errno, 123);
    errno = 0;
    return 0;
""",
    ),
    (
        "stddef",
        "offsetof",
        """
    struct s { char a; int b; };
    RXDK_TEST_TRUE(offsetof(struct s, b) >= 1);
    return 0;
""",
    ),
    (
        "limits",
        "char_bit",
        """
    RXDK_TEST_EQ(CHAR_BIT, 8);
    return 0;
""",
    ),
    (
        "stdint",
        "uint32_width",
        """
    uint32_t x = 0x12345678u;
    RXDK_TEST_EQ((unsigned)(x >> 16), 0x1234u);
    return 0;
""",
    ),
    (
        "stdio",
        "sprintf_int",
        """
    char buf[16];
    RXDK_TEST_EQ(sprintf(buf, "%d", 1234), 4);
    RXDK_TEST_STR_EQ(buf, "1234");
    return 0;
""",
    ),
    (
        "c23",
        "stdbit_leading_zeros",
        """
#if __STDC_VERSION__ >= 202311L
    unsigned x = 0x80000000u;
    RXDK_TEST_EQ(stdc_leading_zeros(x), 0u);
    RXDK_TEST_EQ(stdc_leading_zeros(0u), 32u);
#endif
    return 0;
""",
    ),
]
