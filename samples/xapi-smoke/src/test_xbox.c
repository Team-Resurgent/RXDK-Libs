#include "common.h"

int test_xbox(void)
{
    const DWORD standard = XGetVideoStandard();
    if (standard < XC_VIDEO_STANDARD_NTSC_M || standard > XC_VIDEO_STANDARD_PAL_I) {
        return 1;
    }

    (void)XGetVideoFlags();
    (void)XGetAVPack();
    (void)XGetGameRegion();

    return XAPI_OK;
}
