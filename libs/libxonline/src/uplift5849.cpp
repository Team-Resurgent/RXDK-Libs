// RXDK 5849 uplift: exports XAPI/XOnline entry points that the imported XDK-5849 Live samples
// reference but that are absent from the Jan-2002 leak. Kept minimal and self-contained.

// Include the PUBLIC 5849 headers (like the samples) -- NOT the leak's internal xonp.h, whose older
// decls of a few of these functions would conflict. The force-included bridge already establishes
// the NT/xtl base environment.
#include <xtl.h>
#include <xonline.h>

extern "C" {

// 5849 device (memory-unit / hard-disk) enumeration status query. RXDK enumerates devices
// synchronously, so enumeration is never "busy" -- report idle (0 == XDEVICE_ENUMERATION_IDLE).
XBOXAPI DWORD WINAPI XGetDeviceEnumerationStatus(void)
{
    return 0;
}

// 5849 signature-buffer sizing helper (Xbox.h). Returns the byte size XCalculateSignature produces
// for the given flags. The Xbox content signature is an HMAC-SHA1 (20 bytes). Real signing needs
// the console key; this only sizes the buffer so titles allocate correctly.
XBOXAPI DWORD WINAPI XCalculateSignatureGetSize(DWORD /*dwFlags*/)
{
    return 20;
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

// -------------------------------------------------------------------------------------------------
// 5849 Live API entry points that the Jan-2002 leak does not implement at all (no CXo method to wrap
// -- these are real server round-trips: stats, storage, friends, signatures, silent logon). They are
// documented STUBS so the Live samples link and boot; each returns failure (no task created) and
// must be implemented for real Live use (e.g. Insignia).
// TODO(5849-xonline-live): implement these against the CXo online session / LSP protocol.
// -------------------------------------------------------------------------------------------------

#define RXDK_XO_TASK_STUB(_body) { _body; return E_FAIL; }

// --- Stats ---
XBOXAPI HRESULT WINAPI XOnlineStatWrite(DWORD, const XONLINE_STAT_SPEC *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStatWriteEx(DWORD, const XONLINE_STAT_PROC *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStatWriteGetResult(XONLINETASK_HANDLE, HANDLE *, PXONLINE_STAT_ATTACHMENT_REFERENCE *, DWORD *pdwReferences)
    RXDK_XO_TASK_STUB(if (pdwReferences) *pdwReferences = 0)
XBOXAPI HRESULT WINAPI XOnlineStatRead(DWORD, const XONLINE_STAT_SPEC *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStatReadGetResult(XONLINETASK_HANDLE, DWORD, PXONLINE_STAT_SPEC, DWORD, BYTE *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineStatReset(XUID, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStatLeaderEnumerate(const XUID *, DWORD, DWORD, DWORD, DWORD, const WORD *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStatLeaderEnumerateGetResults(XONLINETASK_HANDLE, DWORD, PXONLINE_STAT_USER, DWORD, PXONLINE_STAT, DWORD *pdwLeaderboardSize, DWORD *pdwReturned, DWORD, BYTE *)
    RXDK_XO_TASK_STUB(if (pdwLeaderboardSize) *pdwLeaderboardSize = 0; if (pdwReturned) *pdwReturned = 0)
XBOXAPI HRESULT WINAPI XOnlineStatUnitEnumerate(XUID, DWORD, XONLINE_STAT_SORTORDER, DWORD, DWORD, const WORD *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStatUnitEnumerateGetResults(XONLINETASK_HANDLE, PXONLINE_STAT_UNIT, DWORD, XONLINE_STAT *, DWORD *pdwReturned)
    RXDK_XO_TASK_STUB(if (pdwReturned) *pdwReturned = 0)
XBOXAPI HRESULT WINAPI XOnlineStatUnitRead(const XUID *, DWORD, const XONLINE_STAT_SPEC_UNIT *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStatUnitReadGetResult(XONLINETASK_HANDLE, DWORD, XONLINE_STAT_SPEC_UNIT *)
    RXDK_XO_TASK_STUB((void)0)

// --- Friends ---
XBOXAPI HRESULT WINAPI XOnlineFriendsGetTitleName(DWORD, DWORD, DWORD dwMaxTitleNameChars, LPWSTR lpTitleName)
    RXDK_XO_TASK_STUB(if (lpTitleName && dwMaxTitleNameChars) lpTitleName[0] = 0)
XBOXAPI HRESULT WINAPI XOnlineFriendsEnumerateFinish(XONLINETASK_HANDLE) { return S_OK; } // cleanup: succeed

// --- Notifications / friends / title ---
XBOXAPI BOOL WINAPI XOnlineGetNotification(DWORD, XONLINE_NOTIFICATION_TYPE) { return FALSE; }
XBOXAPI BOOL WINAPI XOnlineTitleIdIsSameTitle(DWORD) { return FALSE; }
XBOXAPI HRESULT WINAPI XOnlineFriendsGetAcceptedGameInvite(PXONLINE_ACCEPTED_GAMEINVITE pInvite)
    RXDK_XO_TASK_STUB(if (pInvite) memset(pInvite, 0, sizeof(*pInvite)))
XBOXAPI HRESULT WINAPI XOnlineFriendsJoinGame(DWORD, const XONLINE_FRIEND *) { return E_FAIL; }

// --- Silent logon ---
XBOXAPI HRESULT WINAPI XOnlineSilentLogon(const DWORD *, DWORD, HANDLE, PXONLINETASK_HANDLE pHandle)
    RXDK_XO_TASK_STUB(if (pHandle) *pHandle = NULL)

// --- Signatures ---
XBOXAPI HRESULT WINAPI XOnlineSignatureVerify(const XONLINE_SIGNATURE_TO_VERIFY *, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineSignatureVerifyGetResults(XONLINETASK_HANDLE, HRESULT **, DWORD *pdwHresults)
    RXDK_XO_TASK_STUB(if (pdwHresults) *pdwHresults = 0)

// --- Storage ---
XBOXAPI HRESULT WINAPI XOnlineStorageDownload(DWORD, DWORD, LPCWSTR, LPCSTR, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStorageUpload(HANDLE, LPCSTR, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStorageGetProgress(XONLINETASK_HANDLE, DWORD *, ULONGLONG *, ULONGLONG *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineStorageGetInstallLocation(DWORD, LPCWSTR, LPSTR, DWORD *)
    RXDK_XO_TASK_STUB((void)0)

}
