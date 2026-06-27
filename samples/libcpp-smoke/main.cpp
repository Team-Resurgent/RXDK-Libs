#include "tests.hpp"

#include <exception>

#include "xbox/kernel.h"

// Unhandled-exception hook. std::set_terminate is the standard registration
// point; std::terminate fires for an uncaught throw (or if stack unwinding
// can't find a handler / the .eh_frame tables are wrong). Report via DbgPrint
// so it's visible on the debug monitor instead of a silent halt.
static void rxdk_terminate()
{
    DbgPrint("RXDK-LibsZig: std::terminate -- unhandled C++ exception\n");
    for (;;)
        ;
}

// DbgPrint (from xbox/kernel.h) is the kernel debug-console sink; the harness
// reports through it so output matches libc-smoke and lands on the debug
// monitor unbuffered, independent of iostream.

// Map E: -> the hard disk data partition so the <filesystem> test has a real
// place to work. Mounting is policy, so it lives in the harness, not libc.
// STATUS_OBJECT_NAME_COLLISION means E: is already mapped, which is fine.
static void mount_e_drive()
{
    OBJECT_STRING dos;
    OBJECT_STRING dev;
    NTSTATUS s;

    RtlInitAnsiString(&dos, "\\??\\E:");
    RtlInitAnsiString(&dev, "\\Device\\Harddisk0\\Partition1");
    s = IoCreateSymbolicLink(&dos, &dev);
    if (NT_SUCCESS(s) || s == STATUS_OBJECT_NAME_COLLISION)
        DbgPrint("RXDK-LibsZig: mounted E: -> Harddisk0\\Partition1\n");
    else
        DbgPrint("RXDK-LibsZig: mount E: FAILED status=0x%08x\n", (unsigned)s);
}

int main()
{
    std::set_terminate(rxdk_terminate);
    mount_e_drive();

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
