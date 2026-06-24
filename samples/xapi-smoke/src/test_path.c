#include "common.h"

int test_path(void)
{
    if (GetFileAttributesA(XAPI_SMOKE_VOLUME_ROOT) == INVALID_FILE_ATTRIBUTES) {
        return 1;
    }

    ULARGE_INTEGER freeBytes;
    ULARGE_INTEGER totalBytes;
    ULARGE_INTEGER totalFree;
    if (!GetDiskFreeSpaceExA(XAPI_SMOKE_VOLUME_ROOT, &freeBytes, &totalBytes, &totalFree)) {
        return 2;
    }

    if (totalBytes.QuadPart == 0) {
        return 3;
    }

    return XAPI_OK;
}
