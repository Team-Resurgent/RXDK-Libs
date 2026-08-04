// RXDK 5849 uplift: exports XAPI/XOnline entry points that the imported XDK-5849 Live samples
// reference but that are absent from the Jan-2002 leak. Kept minimal and self-contained.

#include "xonp.h"

extern "C" {

// 5849 device (memory-unit / hard-disk) enumeration status query. RXDK enumerates devices
// synchronously, so enumeration is never "busy" -- report idle (0 == XDEVICE_ENUMERATION_IDLE).
XBOXAPI DWORD WINAPI XGetDeviceEnumerationStatus(void)
{
    return 0;
}

// 5849 mid-session logon-user change. Absent from the leak, and a faithful implementation needs the
// online session machinery to add/remove users. For now this is a documented stub that reports
// failure (no task created) so titles link and degrade gracefully; it must be implemented for real
// multi-user Live (e.g. Insignia). TODO(5849-xonline): implement over the CXo logon session.
XBOXAPI HRESULT WINAPI XOnlineChangeLogonUsers(const XONLINE_USER * /*pUsers*/, HANDLE /*hWorkEvent*/,
                                               PXONLINETASK_HANDLE pHandle)
{
    if (pHandle)
        *pHandle = NULL;
    return E_FAIL;
}

}
