#ifndef RXDK_CPP_TESTS_HPP
#define RXDK_CPP_TESTS_HPP

// Conformance test harness types + helpers for libcpp-smoke, mirroring the
// libc-smoke C harness. Each test returns 0 on success or __LINE__ on failure.

struct cpp_test {
    const char *group;
    const char *name;
    int (*run)();
};

unsigned cpp_test_count();
const cpp_test *cpp_tests();

#define RXDK_TEST_FAIL() return __LINE__

#define RXDK_TEST_TRUE(x)     \
    do {                      \
        if (!(x))             \
            RXDK_TEST_FAIL(); \
    } while (0)

#define RXDK_TEST_FALSE(x)    \
    do {                      \
        if (x)                \
            RXDK_TEST_FAIL(); \
    } while (0)

#define RXDK_TEST_EQ(a, b)    \
    do {                      \
        if ((a) != (b))       \
            RXDK_TEST_FAIL(); \
    } while (0)

#define RXDK_TEST_NE(a, b)    \
    do {                      \
        if ((a) == (b))       \
            RXDK_TEST_FAIL(); \
    } while (0)

#endif
