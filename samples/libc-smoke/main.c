#include "xbox/kernel.h"
#include <stdio.h>

typedef struct conformance_test {
    const char *group;
    const char *name;
    int (*run)(void);
} conformance_test;

unsigned conformance_test_count(void);
const conformance_test *conformance_tests(void);

int main(void)
{
    unsigned total = conformance_test_count();
    const conformance_test *tests = conformance_tests();
    unsigned passed = 0;
    unsigned failed = 0;

    DbgPrint("RXDK-LibsZig: libc-smoke start\n");
    printf("RXDK-LibsZig: libc-smoke start\n");

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
