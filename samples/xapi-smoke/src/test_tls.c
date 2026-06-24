#include "common.h"

int test_tls(void)
{
    const DWORD slot = TlsAlloc();
    if (slot == TLS_OUT_OF_INDEXES) {
        return 1;
    }

    const DWORD marker = 0xA5A5A5A5u;
    if (!TlsSetValue(slot, (LPVOID)marker)) {
        TlsFree(slot);
        return 2;
    }

    if (TlsGetValue(slot) != (LPVOID)marker) {
        TlsFree(slot);
        return 3;
    }

    if (!TlsFree(slot)) {
        return 4;
    }

    return XAPI_OK;
}
