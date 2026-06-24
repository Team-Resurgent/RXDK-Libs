#include "xbox/kernel.h"
#include <stdio.h>

typedef struct probe_entry {
    const char *group;
    const char *name;
    int (*run)(void);
} probe_entry;

unsigned kernel_api_probe_count(void);
const probe_entry *kernel_api_probes(void);

int main(void)
{
    unsigned total = kernel_api_probe_count();
    const probe_entry *probes = kernel_api_probes();
    unsigned passed = 0;
    unsigned failed = 0;

#ifdef KERNEL_API_PROBE_DEBUG
    DbgPrint("RXDK-LibsZig: kernel-api-probe debug start\n");
    printf("RXDK-LibsZig: kernel-api-probe debug start\n");
#else
    DbgPrint("RXDK-LibsZig: kernel-api-probe start\n");
    printf("RXDK-LibsZig: kernel-api-probe start\n");
#endif

    for (unsigned i = 0; i < total; i++) {
        DbgPrint("RXDK-LibsZig: probe %s.%s\n", probes[i].group, probes[i].name);
        int rc = probes[i].run();
        DbgPrint("RXDK-LibsZig: probe %s.%s done rc=%d\n", probes[i].group, probes[i].name, rc);
        if (rc == 0) {
            passed++;
        } else {
            failed++;
            DbgPrint("RXDK-LibsZig: kernel-api-probe FAIL %s.%s\n", probes[i].group, probes[i].name);
            printf("RXDK-LibsZig kernel-api-probe FAIL %s.%s\n", probes[i].group, probes[i].name);
        }
    }

#ifdef KERNEL_API_PROBE_DEBUG
    printf(
        "RXDK-LibsZig kernel-api-probe debug OK passed=%u failed=%u total=%u\n",
        passed,
        failed,
        total
    );
    DbgPrint(
        "RXDK-LibsZig kernel-api-probe debug OK passed=%u failed=%u total=%u\n",
        passed,
        failed,
        total
    );
#else
    printf(
        "RXDK-LibsZig kernel-api-probe OK passed=%u failed=%u total=%u\n",
        passed,
        failed,
        total
    );
    DbgPrint(
        "RXDK-LibsZig kernel-api-probe OK passed=%u failed=%u total=%u\n",
        passed,
        failed,
        total
    );
#endif
    for (;;)
        ;
}
