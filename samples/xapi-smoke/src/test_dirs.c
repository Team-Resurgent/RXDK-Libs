#include "common.h"

static const char kDirLeaf[] = "xapi_smoke_dir\\";

int test_dirs(void)
{
    char dirPath[64];
    static const char kProbeLeaf[] = "xapi_smoke_dirprobe.tmp";

    if (!xapi_smoke_pick_writable_path(dirPath, (int)sizeof(dirPath), kProbeLeaf)) {
        return 1;
    }

    char root[8];
    int n = 0;
    for (; dirPath[n] != '\0' && n + 1 < (int)sizeof(root); ++n) {
        root[n] = dirPath[n];
    }
    root[n] = '\0';
    DeleteFileA(dirPath);

    if (!xapi_smoke_build_path(dirPath, (int)sizeof(dirPath), root, kDirLeaf)) {
        return 2;
    }

    RemoveDirectoryA(dirPath);

    if (!CreateDirectoryA(dirPath, NULL)) {
        return 3;
    }

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(dirPath, GetFileExInfoStandard, &fad)) {
        RemoveDirectoryA(dirPath);
        return 4;
    }
    if ((fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        RemoveDirectoryA(dirPath);
        return 5;
    }

    if (!RemoveDirectoryA(dirPath)) {
        return 6;
    }

    return XAPI_OK;
}
