#ifndef RXDK_CONFORMANCE_MINITEST_H
#define RXDK_CONFORMANCE_MINITEST_H

/* Minimal test helpers for kit/runtime conformance (no external framework). */

#define RXDK_TEST_FAIL() return __LINE__

#define RXDK_TEST_EQ(a, b)          \
    do {                            \
        if ((a) != (b))             \
            RXDK_TEST_FAIL();       \
    } while (0)

#define RXDK_TEST_NE(a, b)          \
    do {                            \
        if ((a) == (b))             \
            RXDK_TEST_FAIL();       \
    } while (0)

#define RXDK_TEST_TRUE(x)           \
    do {                            \
        if (!(x))                   \
            RXDK_TEST_FAIL();       \
    } while (0)

#define RXDK_TEST_FALSE(x)          \
    do {                            \
        if (x)                      \
            RXDK_TEST_FAIL();       \
    } while (0)

#define RXDK_TEST_STR_EQ(a, b)      \
    do {                            \
        if (strcmp((a), (b)) != 0)  \
            RXDK_TEST_FAIL();       \
    } while (0)

#endif
