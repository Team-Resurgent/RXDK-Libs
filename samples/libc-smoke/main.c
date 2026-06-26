#include "xbox/kernel.h"
#include <stdio.h>
#include <time.h>

typedef struct conformance_test {
    const char *group;
    const char *name;
    int (*run)(void);
} conformance_test;

unsigned conformance_test_count(void);
const conformance_test *conformance_tests(void);

/*
 * Test-harness setup: map E: to the hard disk's data partition so the
 * filesystem test has somewhere real to write. Mounting is policy, so it lives
 * in the test, not in libc. STATUS_OBJECT_NAME_COLLISION means E: is already
 * mapped, which is fine.
 */
static void mount_e_drive(void)
{
    OBJECT_STRING dos;
    OBJECT_STRING dev;
    NTSTATUS s;

    RtlInitAnsiString(&dos, "\\??\\E:");
    RtlInitAnsiString(&dev, "\\Device\\Harddisk0\\Partition1");
    s = IoCreateSymbolicLink(&dos, &dev);
    if (NT_SUCCESS(s) || s == STATUS_OBJECT_NAME_COLLISION) {
        DbgPrint("RXDK-LibsZig: mounted E: -> Harddisk0\\Partition1\n");
    } else {
        DbgPrint("RXDK-LibsZig: mount E: FAILED status=0x%08x\n", (unsigned)s);
    }
}

/* Show the time stack working end to end: time() -> gmtime() -> strftime(). */
static void print_datetime(void)
{
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    char buf[64];

    if (t && strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", t) > 0) {
        DbgPrint("RXDK-LibsZig: time = %s (epoch %ld)\n", buf, (long)now);
        printf("RXDK-LibsZig: time = %s (epoch %ld)\n", buf, (long)now);
    } else {
        DbgPrint("RXDK-LibsZig: time epoch=%ld\n", (long)now);
    }
}

int main(void)
{
    unsigned total = conformance_test_count();
    const conformance_test *tests = conformance_tests();
    unsigned passed = 0;
    unsigned failed = 0;

    DbgPrint("RXDK-LibsZig: libc-smoke start\n");
    printf("RXDK-LibsZig: libc-smoke start\n");

    mount_e_drive();
    print_datetime();

    for (unsigned i = 0; i < total; i++) {
        DbgPrint("RXDK-LibsZig: test %s.%s\n", tests[i].group, tests[i].name);
        int rc = tests[i].run();
        if (rc == 0) {
            passed++;
        } else {
            failed++;
            DbgPrint(
                "RXDK-LibsZig: libc-smoke FAIL %s.%s line=%d\n",
                tests[i].group,
                tests[i].name,
                rc
            );
            printf(
                "RXDK-LibsZig libc-smoke FAIL %s.%s line=%d\n",
                tests[i].group,
                tests[i].name,
                rc
            );
        }
    }

    printf(
        "RXDK-LibsZig libc-smoke OK passed=%u failed=%u total=%u\n",
        passed,
        failed,
        total
    );
    DbgPrint(
        "RXDK-LibsZig libc-smoke OK passed=%u failed=%u total=%u\n",
        passed,
        failed,
        total
    );
    for (;;)
        ;
}
