#include "common.h"

static const char kPayload[] = "xapi-smoke-ok";
static const char kLeaf[] = "xapi_smoke.tmp";

int test_files(void)
{
    char smokePath[64];

    if (!xapi_smoke_pick_writable_path(smokePath, (int)sizeof(smokePath), kLeaf)) {
        return 1;
    }

    xapi_smoke_trace_line2("files path ", smokePath);

    DeleteFileA(smokePath);

    HANDLE file = CreateFileA(
        smokePath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return 2;
    }

    DWORD written = 0;
    if (!WriteFile(file, kPayload, (DWORD)(sizeof(kPayload) - 1), &written, NULL)
        || written != (DWORD)(sizeof(kPayload) - 1)) {
        CloseHandle(file);
        DeleteFileA(smokePath);
        return 3;
    }

    if (SetFilePointer(file, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        CloseHandle(file);
        DeleteFileA(smokePath);
        return 4;
    }

    char buf[16];
    DWORD read = 0;
    if (!ReadFile(file, buf, (DWORD)(sizeof(kPayload) - 1), &read, NULL)
        || read != (DWORD)(sizeof(kPayload) - 1)) {
        CloseHandle(file);
        DeleteFileA(smokePath);
        return 5;
    }
    buf[read] = '\0';
    if (lstrcmpA(buf, kPayload) != 0) {
        CloseHandle(file);
        DeleteFileA(smokePath);
        return 6;
    }

    DWORD size = GetFileSize(file, NULL);
    if (size != (DWORD)(sizeof(kPayload) - 1)) {
        CloseHandle(file);
        DeleteFileA(smokePath);
        return 7;
    }

    CloseHandle(file);

    if (!DeleteFileA(smokePath)) {
        return 8;
    }

    return XAPI_OK;
}
