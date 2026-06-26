#include "tests.hpp"

// DbgPrint is the kernel debug-console sink (extern "C"); the harness reports
// through it so output matches libc-smoke and lands on the debug monitor
// unbuffered, independent of iostream.
extern "C" void DbgPrint(const char *format, ...);

int main()
{
    unsigned total = cpp_test_count();
    const cpp_test *tests = cpp_tests();
    unsigned passed = 0;
    unsigned failed = 0;

    DbgPrint("RXDK-LibsZig: libcpp-smoke start\n");

    for (unsigned i = 0; i < total; ++i) {
        DbgPrint("RXDK-LibsZig: test %s.%s\n", tests[i].group, tests[i].name);
        int rc = tests[i].run();
        if (rc == 0) {
            ++passed;
        } else {
            ++failed;
            DbgPrint(
                "RXDK-LibsZig: libcpp-smoke FAIL %s.%s line=%d\n",
                tests[i].group,
                tests[i].name,
                rc
            );
        }
    }

    DbgPrint(
        "RXDK-LibsZig libcpp-smoke OK passed=%u failed=%u total=%u\n",
        passed,
        failed,
        total
    );
    for (;;)
        ;
}
