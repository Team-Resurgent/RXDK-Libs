#include "common.h"

static const char kPayload[] = "xapi-copy-ok";
static const char kSrcLeaf[] = "xapi_smoke_src.tmp";
static const char kDstLeaf[] = "xapi_smoke_dst.tmp";
static const char kMvLeaf[] = "xapi_smoke_mv.tmp";

int test_copyfile(void)
{
    char srcPath[64];
    char dstPath[64];
    char mvPath[64];

    if (!xapi_smoke_pick_writable_path(srcPath, (int)sizeof(srcPath), kSrcLeaf)) {
        return 1;
    }

    char root[8];
    int n = 0;
    for (; srcPath[n] != '\0' && n + 1 < (int)sizeof(root); ++n) {
        root[n] = srcPath[n];
    }
    root[n] = '\0';

    if (!xapi_smoke_build_path(dstPath, (int)sizeof(dstPath), root, kDstLeaf)
        || !xapi_smoke_build_path(mvPath, (int)sizeof(mvPath), root, kMvLeaf)) {
        return 2;
    }

    DeleteFileA(srcPath);
    DeleteFileA(dstPath);
    DeleteFileA(mvPath);

    HANDLE file = CreateFileA(
        srcPath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return 3;
    }

    DWORD written = 0;
    const DWORD payloadLen = (DWORD)(sizeof(kPayload) - 1);
    if (!WriteFile(file, kPayload, payloadLen, &written, NULL) || written != payloadLen) {
        CloseHandle(file);
        DeleteFileA(srcPath);
        return 4;
    }
    CloseHandle(file);

    if (!CopyFileA(srcPath, dstPath, FALSE)) {
        DeleteFileA(srcPath);
        return 5;
    }

    file = CreateFileA(dstPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(srcPath);
        DeleteFileA(dstPath);
        return 6;
    }

    char buf[16];
    DWORD read = 0;
    if (!ReadFile(file, buf, payloadLen, &read, NULL) || read != payloadLen) {
        CloseHandle(file);
        DeleteFileA(srcPath);
        DeleteFileA(dstPath);
        return 7;
    }
    CloseHandle(file);
    buf[payloadLen] = '\0';
    if (lstrcmpA(buf, kPayload) != 0) {
        DeleteFileA(srcPath);
        DeleteFileA(dstPath);
        return 8;
    }

    if (!MoveFileA(dstPath, mvPath)) {
        DeleteFileA(srcPath);
        DeleteFileA(dstPath);
        return 9;
    }

    if (GetFileAttributesA(dstPath) != INVALID_FILE_ATTRIBUTES) {
        DeleteFileA(srcPath);
        DeleteFileA(mvPath);
        return 10;
    }

    DeleteFileA(srcPath);
    DeleteFileA(mvPath);
    return XAPI_OK;
}
