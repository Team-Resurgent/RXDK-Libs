#include "common.h"

int test_error(void)
{
    SetLastError(ERROR_INVALID_PARAMETER);
    if (GetLastError() != ERROR_INVALID_PARAMETER) {
        return 1;
    }
    SetLastError(0);
    if (GetLastError() != 0) {
        return 2;
    }
    return XAPI_OK;
}
