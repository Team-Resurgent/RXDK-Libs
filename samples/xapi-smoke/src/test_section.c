#include "common.h"

static BOOL section_handle_valid(HANDLE section)
{
    return section != NULL && section != INVALID_HANDLE_VALUE;
}

int test_section(void)
{
    /* Every imagebld XBE has a .text section; D3D is absent in NOD3D smoke builds. */
    HANDLE section = XGetSectionHandle(".text");
    if (!section_handle_valid(section)) {
        return XAPI_SKIP;
    }

    const DWORD size = XGetSectionSize(section);
    if (size == 0) {
        return 1;
    }

    PVOID base = XLoadSectionByHandle(section);
    if (!base) {
        return 2;
    }

    if (!XFreeSectionByHandle(section)) {
        return 3;
    }

    return XAPI_OK;
}
