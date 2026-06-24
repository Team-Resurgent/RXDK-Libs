#include "common.h"

int test_content(void)
{
    char root[8];
    if (!xapi_smoke_pick_save_root(root)) {
        return XAPI_SKIP;
    }

    XCONTENT_FIND_DATA findData;
    HANDLE find = XFindFirstContent(root, 0, &findData);
    if (find == INVALID_HANDLE_VALUE) {
        return XAPI_SKIP;
    }

    XFindClose(find);
    return XAPI_OK;
}
