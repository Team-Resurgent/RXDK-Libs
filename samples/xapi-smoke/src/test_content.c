#include "common.h"

static const WCHAR kContentName[] = L"xapi_smoke";
static const XOFFERING_ID kOfferingId = 1;
static const DWORD kContentFlags = 0;

static BOOL xapi_smoke_ensure_content_tree(const char* package_dir)
{
    char parent[MAX_PATH];

    if (!xapi_smoke_build_path(parent, (int)sizeof(parent), "T:\\", "$C")) {
        return FALSE;
    }

    if (!CreateDirectoryA(parent, NULL)) {
        const DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) {
            return FALSE;
        }
    }

    if (!CreateDirectoryA(package_dir, NULL)) {
        const DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) {
            return FALSE;
        }
    }

    return TRUE;
}

int test_content(void)
{
    char root[8];
    char package_dir[MAX_PATH];
    char package_dir_no_slash[MAX_PATH];

    if (!xapi_smoke_pick_content_root(root)) {
        xapi_smoke_trace_line("content no T:");
        return XAPI_SKIP;
    }
    xapi_smoke_trace_line2("content root ", root);

    if (!xapi_smoke_build_content_package_path(
            package_dir,
            (int)sizeof(package_dir),
            root,
            (unsigned long)kOfferingId,
            kContentFlags)) {
        return 1;
    }

    lstrcpynA(package_dir_no_slash, package_dir, (int)sizeof(package_dir_no_slash));
    {
        const int len = lstrlenA(package_dir_no_slash);
        if (len > 0 && package_dir_no_slash[len - 1] == '\\') {
            package_dir_no_slash[len - 1] = '\0';
        }
    }

    if (!xapi_smoke_ensure_content_tree(package_dir_no_slash)) {
        return 2;
    }

    if (!XCreateContentSimple(0, kOfferingId, kContentFlags, kContentName, package_dir_no_slash)) {
        (void)XRemoveContent(package_dir_no_slash);
        return 3;
    }

    XCONTENT_FIND_DATA findData;
    HANDLE find = XFindFirstContent(root, 0, &findData);
    if (find == INVALID_HANDLE_VALUE) {
        (void)XRemoveContent(package_dir_no_slash);
        return 4;
    }
    XFindClose(find);

    if (!XRemoveContent(package_dir_no_slash)) {
        return 5;
    }

    return XAPI_OK;
}
