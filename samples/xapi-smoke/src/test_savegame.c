#include "common.h"

static const WCHAR kSaveName[] = L"xapi_smoke";

int test_savegame(void)
{
    char root[8];
    if (!xapi_smoke_pick_save_root(root)) {
        return XAPI_SKIP;
    }

    char savePath[MAX_PATH];
    const DWORD createErr = XCreateSaveGame(
        root,
        kSaveName,
        OPEN_ALWAYS,
        0,
        savePath,
        (UINT)sizeof(savePath));
    if (createErr != ERROR_SUCCESS) {
        if (xapi_smoke_save_root_uses_mu(root)) {
            xapi_smoke_unmount_mu_port0();
        }
        return 1;
    }

    XGAME_FIND_DATA findData;
    HANDLE find = XFindFirstSaveGame(root, &findData);
    if (find == INVALID_HANDLE_VALUE) {
        (void)XDeleteSaveGame(root, kSaveName);
        if (xapi_smoke_save_root_uses_mu(root)) {
            xapi_smoke_unmount_mu_port0();
        }
        return 2;
    }
    XFindClose(find);

    if (XDeleteSaveGame(root, kSaveName) != ERROR_SUCCESS) {
        if (xapi_smoke_save_root_uses_mu(root)) {
            xapi_smoke_unmount_mu_port0();
        }
        return 3;
    }

    if (xapi_smoke_save_root_uses_mu(root)) {
        xapi_smoke_unmount_mu_port0();
    }

    return XAPI_OK;
}
