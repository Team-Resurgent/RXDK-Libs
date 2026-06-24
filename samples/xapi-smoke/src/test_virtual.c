#include "common.h"

int test_virtual(void)
{
    void* page = VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_READWRITE);
    if (!page) {
        return 1;
    }

    *(volatile DWORD*)page = 0x12345678u;
    if (*(volatile DWORD*)page != 0x12345678u) {
        VirtualFree(page, 0, MEM_RELEASE);
        return 2;
    }

    if (!VirtualFree(page, 0, MEM_RELEASE)) {
        return 3;
    }

    return XAPI_OK;
}
