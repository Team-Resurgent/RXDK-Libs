#include "common.h"

static const WCHAR kNickname[] = L"xapi";

int test_nickname(void)
{
    if (!XSetNickname(kNickname, FALSE)) {
        return XAPI_SKIP;
    }

    WCHAR found[MAX_NICKNAME];
    HANDLE find = XFindFirstNicknameW(TRUE, found, MAX_NICKNAME);
    if (find == INVALID_HANDLE_VALUE) {
        return 1;
    }

    XFindClose(find);
    return XAPI_OK;
}
