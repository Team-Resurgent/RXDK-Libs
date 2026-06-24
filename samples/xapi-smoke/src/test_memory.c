#include "common.h"

int test_memory(void)
{
    char src[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    char dst[8];

    CopyMemory(dst, src, sizeof(dst));
    for (unsigned i = 0; i < sizeof(dst); ++i) {
        if (dst[i] != src[i]) {
            return 1;
        }
    }

    ZeroMemory(dst, sizeof(dst));
    for (unsigned i = 0; i < sizeof(dst); ++i) {
        if (dst[i] != 0) {
            return 2;
        }
    }

    HANDLE heap = GetProcessHeap();
    if (!heap) {
        return 3;
    }

    void* block = HeapAlloc(heap, 0, 64);
    if (!block) {
        return 4;
    }

    if (HeapSize(heap, 0, block) < 64) {
        HeapFree(heap, 0, block);
        return 5;
    }

    void* grown = HeapReAlloc(heap, 0, block, 128);
    if (!grown) {
        HeapFree(heap, 0, block);
        return 6;
    }

    if (!HeapFree(heap, 0, grown)) {
        return 7;
    }

    return XAPI_OK;
}
