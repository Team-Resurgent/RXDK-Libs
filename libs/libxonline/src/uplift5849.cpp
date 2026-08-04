// RXDK 5849 uplift: exports XAPI/XOnline entry points that the imported XDK-5849 Live samples
// reference but that are absent from the Jan-2002 leak. Kept minimal and self-contained.

// Include the PUBLIC 5849 headers (like the samples) -- NOT the leak's internal xonp.h, whose older
// decls of a few of these functions would conflict. The force-included bridge already establishes
// the NT/xtl base environment.
#include <xtl.h>
#include <xonline.h>

#include <string.h> // memset (zero-filled out-params in the stubs below)

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

// 5849 mid-session logon-user change (REAL). The leak has no dedicated
// change-users path, but its XOnlineLogon already tears down and replaces a
// previous logon session -- which is exactly what changing the user set means:
// re-logon with the new users and the same services as the current session.
static DWORD RxdkGetCapturedServices(const DWORD **ppdwServiceIDs); // defined with the capture globals below

XBOXAPI HRESULT WINAPI XOnlineChangeLogonUsers(const XONLINE_USER *pUsers, HANDLE hWorkEvent,
                                               PXONLINETASK_HANDLE pHandle)
{
    if (pHandle)
        *pHandle = NULL;
    if (!pUsers)
        return E_INVALIDARG;

    const DWORD *pdwServiceIDs = NULL;
    DWORD cServices = RxdkGetCapturedServices(&pdwServiceIDs);
    return XOnlineLogon(pUsers, cServices ? pdwServiceIDs : NULL, cServices, hWorkEvent, pHandle);
}

// -------------------------------------------------------------------------------------------------
// 5849 Live services whose WIRE PROTOCOLS do not exist in the Jan-2002 leak (stats v2, storage,
// messaging, arbitration, competition, teams, mutelist, offerings, query). These entry points
// implement the "service unavailable" behavior: each fails cleanly with no task created, which is
// the same thing retail reports when the corresponding service is down or absent from the logon's
// service list -- titles are required to handle it. This is a protocol boundary, not unfinished
// work: implementing them for real needs the 5849 LSP protocol documentation (or an Insignia-side
// definition of these services), neither of which exists in the leak.
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

// --- Notifications / friends / title --- (REAL)

// 5849's notification query. The leak's friends machinery already carries the
// notification-worthy state in each friend's dwFriendState (received request /
// received invite / invite answered), refreshed by the friends enumeration task
// the title (or the UIX engine) keeps pumping -- so derive the notifications
// from the live friends snapshot. NEW_GAME_INVITE reports invites that appeared
// since it was last queried for that user.
XBOXAPI BOOL WINAPI XOnlineGetNotification(DWORD dwUserIndex, XONLINE_NOTIFICATION_TYPE NotificationType)
{
    static DWORD s_dwSeenInvites[XONLINE_MAX_LOGON_USERS]; // per-user invite count at last NEW query

    if (dwUserIndex >= XONLINE_MAX_LOGON_USERS)
        return FALSE;

    static XONLINE_FRIEND s_friends[MAX_FRIENDS];
    DWORD cFriends = XOnlineFriendsGetLatest(dwUserIndex, MAX_FRIENDS, s_friends);

    DWORD cRequests = 0, cInvites = 0, cAnswers = 0;
    for (DWORD i = 0; i < cFriends; i++) {
        DWORD s = s_friends[i].dwFriendState;
        if (s & XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST)
            cRequests++;
        if (s & XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE)
            cInvites++;
        if (s & (XONLINE_FRIENDSTATE_FLAG_INVITEACCEPTED | XONLINE_FRIENDSTATE_FLAG_INVITEREJECTED))
            cAnswers++;
    }

    switch (NotificationType) {
    case XONLINE_NOTIFICATION_FRIEND_REQUEST:
        return cRequests != 0;
    case XONLINE_NOTIFICATION_GAME_INVITE:
        return cInvites != 0;
    case XONLINE_NOTIFICATION_NEW_GAME_INVITE:
    {
        BOOL fNew = cInvites > s_dwSeenInvites[dwUserIndex];
        s_dwSeenInvites[dwUserIndex] = cInvites;
        return fNew;
    }
    case XONLINE_NOTIFICATION_GAME_INVITE_ANSWER:
        return cAnswers != 0;
    default:
        return FALSE;
    }
}

// Compare a title ID against our own, read from the XBE certificate (header at
// the standard base 0x10000; the certificate pointer lives at +0x118 and
// TitleID at certificate +8 -- same fields libxapi's bootutil.c reads through
// XeImageHeader()->Certificate->TitleID).
XBOXAPI BOOL WINAPI XOnlineTitleIdIsSameTitle(DWORD dwTitleId)
{
    DWORD dwCertificate = *(const DWORD *)0x10118;
    DWORD dwOwnTitleId  = *(const DWORD *)(dwCertificate + 8);
    return dwTitleId == dwOwnTitleId;
}
XBOXAPI HRESULT WINAPI XOnlineFriendsGetAcceptedGameInvite(PXONLINE_ACCEPTED_GAMEINVITE pInvite)
    RXDK_XO_TASK_STUB(if (pInvite) memset(pInvite, 0, sizeof(*pInvite)))
XBOXAPI HRESULT WINAPI XOnlineFriendsJoinGame(DWORD, const XONLINE_FRIEND *) { return E_FAIL; }

// --- Logon-state save/restore --- (REAL)
// The 5849 logon state is a title-persisted snapshot of the current logon
// (users + requested services) used to hand a session across XLaunchNewImage.
// Users come from the live logon session (XOnlineGetLogonUsers); the service
// IDs are captured at XOnlineLogon time (see RxdkCaptureLogonServices below --
// the leak keeps them only inside the opaque logon task). Layout of Data[]
// follows XONLINE_LOGON_STATE_SIZE: MAX_LOGON_USERS XONLINE_USERs, then
// MAX_LOGON_STATE_SERVICES DWORD service IDs (zero-terminated).

static DWORD s_rxdkLogonServiceIDs[XONLINE_MAX_LOGON_STATE_SERVICES];
static DWORD s_rxdkLogonServiceCount;

XBOXAPI VOID WINAPI RxdkCaptureLogonServices(const DWORD *pdwServiceIDs, DWORD cServices)
{
    memset(s_rxdkLogonServiceIDs, 0, sizeof(s_rxdkLogonServiceIDs));
    if (cServices > XONLINE_MAX_LOGON_STATE_SERVICES)
        cServices = XONLINE_MAX_LOGON_STATE_SERVICES;
    for (DWORD i = 0; i < cServices; i++)
        s_rxdkLogonServiceIDs[i] = pdwServiceIDs[i];
    s_rxdkLogonServiceCount = cServices;
}

static DWORD RxdkGetCapturedServices(const DWORD **ppdwServiceIDs)
{
    *ppdwServiceIDs = s_rxdkLogonServiceIDs;
    return s_rxdkLogonServiceCount;
}

XBOXAPI HRESULT WINAPI XOnlineRetrieveLogonState(const XONLINE_LOGON_STATE *pLogonState,
                                                 PXONLINE_USER pUsers, DWORD *pdwServiceIDs,
                                                 DWORD *pdwServices)
{
    if (!pLogonState || !pdwServices)
        return E_INVALIDARG;
    if (pLogonState->bType != XONLINE_LOGON_STATE_TYPE ||
        pLogonState->bVersion != XONLINE_LOGON_STATE_VERSION ||
        pLogonState->cbSize != XONLINE_LOGON_STATE_SIZE)
        return E_INVALIDARG;

    const XONLINE_USER *pStateUsers = (const XONLINE_USER *)pLogonState->Data;
    const DWORD *pStateServices =
        (const DWORD *)(pLogonState->Data + XONLINE_MAX_LOGON_USERS * sizeof(XONLINE_USER));

    if (pUsers)
        memcpy(pUsers, pStateUsers, XONLINE_MAX_LOGON_USERS * sizeof(XONLINE_USER));

    DWORD cServices = 0;
    while (cServices < XONLINE_MAX_LOGON_STATE_SERVICES && pStateServices[cServices])
        cServices++;
    if (pdwServiceIDs) {
        DWORD cCopy = (*pdwServices && *pdwServices < cServices) ? *pdwServices : cServices;
        memcpy(pdwServiceIDs, pStateServices, cCopy * sizeof(DWORD));
    }
    *pdwServices = cServices;
    return S_OK;
}

// --- Matchmaking / competition ---
XBOXAPI HRESULT WINAPI XOnlineQuerySearch(DWORD, DWORD, DWORD, DWORD, DWORD, const XONLINE_ATTRIBUTE_SPEC *, DWORD, const XONLINE_ATTRIBUTE *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI DWORD WINAPI XOnlineMatchSearchResultsLen(DWORD, DWORD, const XONLINE_ATTRIBUTE_SPEC *) { return 0; }
XBOXAPI HRESULT WINAPI XOnlineCompetitionTopology(DWORD, ULONGLONG, DWORD, DWORD, DWORD, DWORD, DWORD, const XONLINE_ATTRIBUTE_SPEC *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)

// --- Silent logon --- (REAL: wraps the leak's XOnlineLogon)
// 5849 titles use this to sign in without UI. Retail reads the dashboard's
// persistent "last logged on" store, which does not exist in the RXDK world
// (titles that want session handoff use XOnlineSaveLogonState/
// XOnlineRetrieveLogonState, both real above). RXDK policy: sign in the FIRST
// stored account (hard disk / MU) -- the same account the UIX picker defaults to.
XBOXAPI HRESULT WINAPI XOnlineSilentLogon(const DWORD *pdwServiceIDs, DWORD dwServices,
                                          HANDLE hWorkEvent, PXONLINETASK_HANDLE pHandle)
{
    if (pHandle)
        *pHandle = NULL;

    XONLINE_USER storedUsers[XONLINE_MAX_STORED_ONLINE_USERS];
    DWORD cStored = 0;
    HRESULT hr = XOnlineGetUsers(storedUsers, &cStored);
    if (FAILED(hr))
        return hr;
    if (cStored == 0)
        return XONLINE_E_USER_NOT_PRESENT;

    XONLINE_USER logonUsers[XONLINE_MAX_LOGON_USERS];
    memset(logonUsers, 0, sizeof(logonUsers));
    logonUsers[0] = storedUsers[0];

    return XOnlineLogon(logonUsers, pdwServiceIDs, dwServices, hWorkEvent, pHandle);
}

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
XBOXAPI HRESULT WINAPI XOnlineStorageEnumerate(DWORD, DWORD, LPCWSTR, DWORD, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStorageEnumerateGetResults(XONLINETASK_HANDLE, DWORD *pdwTotalResults, DWORD *pdwResultsReturned, PXONLINESTORAGE_FILE_INFO **prgpStorageFileInfo)
    RXDK_XO_TASK_STUB(if (pdwTotalResults) *pdwTotalResults = 0; if (pdwResultsReturned) *pdwResultsReturned = 0; if (prgpStorageFileInfo) *prgpStorageFileInfo = NULL)
XBOXAPI HRESULT WINAPI XOnlineStorageUploadByServerPath(DWORD, DWORD, LPCWSTR, FILETIME, LPCSTR, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)

// --- Offerings / content install (5849 renamed content-download surface) ---
XBOXAPI HRESULT WINAPI XOnlineContentInstall(XOFFERING_ID, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineOfferingCancel(DWORD, XOFFERING_ID, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineOfferingDetails(DWORD, XOFFERING_ID, DWORD, DWORD, PBYTE, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineOfferingDetailsGetResults(XONLINETASK_HANDLE, PXONLINEOFFERING_DETAILS pDetails)
    RXDK_XO_TASK_STUB(if (pDetails) memset(pDetails, 0, sizeof(*pDetails)))
XBOXAPI HRESULT WINAPI XOnlineOfferingEnumerate(DWORD, const XONLINEOFFERING_ENUM_PARAMS *, PBYTE, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineOfferingIsNewContentAvailable(DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineOfferingPurchase(DWORD, XOFFERING_ID, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)

// --- Logon-state save --- (REAL; see XOnlineRetrieveLogonState above)
XBOXAPI HRESULT WINAPI XOnlineSaveLogonState(PXONLINE_LOGON_STATE pLogonState)
{
    if (!pLogonState)
        return E_INVALIDARG;

    PXONLINE_USER pLogonUsers = XOnlineGetLogonUsers();
    if (!pLogonUsers)
        return XONLINE_E_USER_NOT_LOGGED_ON;

    memset(pLogonState, 0, sizeof(*pLogonState));
    pLogonState->bType    = XONLINE_LOGON_STATE_TYPE;
    pLogonState->bVersion = XONLINE_LOGON_STATE_VERSION;
    pLogonState->cbSize   = XONLINE_LOGON_STATE_SIZE;

    memcpy(pLogonState->Data, pLogonUsers, XONLINE_MAX_LOGON_USERS * sizeof(XONLINE_USER));
    memcpy(pLogonState->Data + XONLINE_MAX_LOGON_USERS * sizeof(XONLINE_USER),
           s_rxdkLogonServiceIDs, sizeof(s_rxdkLogonServiceIDs));
    return S_OK;
}

// --- Messaging / notifications ---
XBOXAPI HRESULT WINAPI XOnlineMessageCreate(BYTE, WORD, WORD, ULONGLONG, DWORD, WORD, XONLINE_MSG_HANDLE *phMsg)
    RXDK_XO_TASK_STUB(if (phMsg) *phMsg = NULL)
XBOXAPI HRESULT WINAPI XOnlineMessageDestroy(XONLINE_MSG_HANDLE) { return S_OK; } // cleanup: succeed
XBOXAPI HRESULT WINAPI XOnlineMessageDelete(DWORD, DWORD, BOOL)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineMessageDetails(DWORD, DWORD, DWORD, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineMessageDetailsGetResultsProperty(XONLINETASK_HANDLE, WORD, DWORD, PVOID, DWORD *pdwPropValueSize, DWORD *pdwAttachmentFlags)
    RXDK_XO_TASK_STUB(if (pdwPropValueSize) *pdwPropValueSize = 0; if (pdwAttachmentFlags) *pdwAttachmentFlags = 0)
XBOXAPI HRESULT WINAPI XOnlineMessageDownloadAttachmentToMemory(XONLINETASK_HANDLE, WORD, PBYTE, DWORD, HANDLE, PXONLINETASK_HANDLE phDownloadTask)
    RXDK_XO_TASK_STUB(if (phDownloadTask) *phDownloadTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineMessageDownloadAttachmentGetProgress(XONLINETASK_HANDLE, DWORD *pdwPercentDone, ULONGLONG *pqwNumerator, ULONGLONG *pqwDenominator)
    RXDK_XO_TASK_STUB(if (pdwPercentDone) *pdwPercentDone = 0; if (pqwNumerator) *pqwNumerator = 0; if (pqwDenominator) *pqwDenominator = 0)
XBOXAPI HRESULT WINAPI XOnlineMessageEnumerate(DWORD, XONLINE_MSG_SUMMARY *, DWORD *pdwNumMsgSummaries)
    RXDK_XO_TASK_STUB(if (pdwNumMsgSummaries) *pdwNumMsgSummaries = 0)
XBOXAPI HRESULT WINAPI XOnlineMessageSend(DWORD, XONLINE_MSG_HANDLE, DWORD, const XUID *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineMessageSendGetProgress(XONLINETASK_HANDLE, DWORD *pdwPercentDone, ULONGLONG *pqwNumerator, ULONGLONG *pqwDenominator)
    RXDK_XO_TASK_STUB(if (pdwPercentDone) *pdwPercentDone = 0; if (pqwNumerator) *pqwNumerator = 0; if (pqwDenominator) *pqwDenominator = 0)
XBOXAPI HRESULT WINAPI XOnlineMessageSetFlags(DWORD, DWORD, DWORD, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineMessageSetProperty(XONLINE_MSG_HANDLE, WORD, DWORD, const VOID *, DWORD)
    RXDK_XO_TASK_STUB((void)0)
// (XOnlineNotificationSetState is already implemented by the leak's presence/notification code.)
XBOXAPI BOOL WINAPI XOnlineGetNotificationEx(DWORD, PXONLINE_NOTIFICATION_EX_INFO pNotificationInfo, DWORD *pdwStateFlags)
{
    if (pNotificationInfo)
        memset(pNotificationInfo, 0, sizeof(*pNotificationInfo));
    if (pdwStateFlags)
        *pdwStateFlags = 0;
    return FALSE; // no notification pending
}


// --- Arbitration / competition / teams / presence / query / storage-to-memory / sessions ---
// (the Integrated demo exercises the whole 5849 Live surface)
XBOXAPI HRESULT WINAPI XOnlineArbitrationCreateRoundID(ULONGLONG *pqwRoundID)
    RXDK_XO_TASK_STUB(if (pqwRoundID) *pqwRoundID = 0)
XBOXAPI HRESULT WINAPI XOnlineArbitrationRegister(const XONLINE_ARB_ID *, WORD, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineArbitrationRegisterGetResults(XONLINETASK_HANDLE, DWORD, XONLINE_ARB_REGISTRANT *, DWORD *pdwNumRegistrants)
    RXDK_XO_TASK_STUB(if (pdwNumRegistrants) *pdwNumRegistrants = 0)
XBOXAPI HRESULT WINAPI XOnlineArbitrationReport(const XONLINE_ARB_ID *, DWORD, const XONLINE_STAT_PROC *, const XONLINE_ARB_REPORT_DATA *, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineCompetitionCancel(DWORD, DWORD, ULONGLONG, ULONGLONG, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineCompetitionCheckin(DWORD, DWORD, ULONGLONG, ULONGLONG, ULONGLONG, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineCompetitionCreateGetResults(XONLINETASK_HANDLE, PXONLINE_COMP_CREATE_RESULTS)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineCompetitionCreateSingleElimination(DWORD, DWORD, ULONGLONG, const XONLINE_COMP_SINGLE_ELIMINATION_ATTRIBUTES *, DWORD, const XONLINE_ATTRIBUTE *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineCompetitionManageEntrant(DWORD, DWORD, DWORD, ULONGLONG, ULONGLONG, DWORD, const XONLINE_ATTRIBUTE *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineCompetitionSearch(DWORD, DWORD, DWORD, DWORD, DWORD, const XONLINE_ATTRIBUTE *, DWORD, const XONLINE_ATTRIBUTE_SPEC *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineCompetitionSearchGetResults(XONLINETASK_HANDLE, DWORD *pdwTotalItemsInSearchResult, DWORD *pdwItemsReturned, DWORD *pdwResultBufferSize, PBYTE)
    RXDK_XO_TASK_STUB(if (pdwTotalItemsInSearchResult) *pdwTotalItemsInSearchResult = 0; if (pdwItemsReturned) *pdwItemsReturned = 0; if (pdwResultBufferSize) *pdwResultBufferSize = 0)
XBOXAPI HRESULT WINAPI XOnlineCompetitionSessionRegister(const XONLINE_ARB_ID *, WORD, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineCompetitionSessionRegisterGetResults(XONLINETASK_HANDLE, DWORD, XONLINE_ARB_REGISTRANT *, DWORD *pdwNumRegistrants)
    RXDK_XO_TASK_STUB(if (pdwNumRegistrants) *pdwNumRegistrants = 0)
XBOXAPI HRESULT WINAPI XOnlineCompetitionSubmitResults(DWORD, ULONGLONG, const XONLINE_ARB_ID *, DWORD, const XONLINE_ARB_REPORT_DATA *, DWORD, const XONLINE_STAT_PROC *, DWORD, const XONLINE_ATTRIBUTE *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineCompetitionTopologyGetResults(XONLINETASK_HANDLE, DWORD *pdwTotalItemsInSearchResult, DWORD *pdwItemsReturned, DWORD *pdwResultBufferSize, PBYTE)
    RXDK_XO_TASK_STUB(if (pdwTotalItemsInSearchResult) *pdwTotalItemsInSearchResult = 0; if (pdwItemsReturned) *pdwItemsReturned = 0; if (pdwResultBufferSize) *pdwResultBufferSize = 0)
// NAT detection needs the Live NAT-type-detection service; report OPEN (the
// benign default -- titles use it only to warn about restrictive NATs).
XBOXAPI XONLINE_NAT_TYPE WINAPI XOnlineGetNatType(void) { return XONLINE_NAT_OPEN; }
XBOXAPI HRESULT WINAPI XOnlineGetSession(XNADDR *, XNKID *, XNKEY *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineGetUserSession(DWORD, XUID, HANDLE, PXONLINETASK_HANDLE phTask, PXONLINE_PEER_SESSION_RESULTS)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineMessageDownloadAttachmentToMemoryGetResults(XONLINETASK_HANDLE, PBYTE *, DWORD *pdwReceivedDataSize, DWORD *pdwTotalDataSize)
    RXDK_XO_TASK_STUB(if (pdwReceivedDataSize) *pdwReceivedDataSize = 0; if (pdwTotalDataSize) *pdwTotalDataSize = 0)
XBOXAPI HRESULT WINAPI XOnlinePresenceAdd(XONLINETASK_HANDLE, DWORD, DWORD, XUID *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlinePresenceInit(DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlinePresenceSubmit(XONLINETASK_HANDLE)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineQueryAdd(DWORD, ULONGLONG, DWORD, DWORD, const XONLINE_ATTRIBUTE *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineQueryAddGetResults(XONLINETASK_HANDLE, XENTITY_ID *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI DWORD WINAPI XOnlineQueryGetResultsBufferSize(DWORD, DWORD, const XONLINE_ATTRIBUTE_SPEC *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineQueryRemove(DWORD, ULONGLONG, DWORD, DWORD, DWORD, const XONLINE_ATTRIBUTE *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineQuerySearchGetResults(XONLINETASK_HANDLE, DWORD *pdwTotalResults, DWORD *pdwReturnedResults, DWORD *pdwResultsSize, PBYTE)
    RXDK_XO_TASK_STUB(if (pdwTotalResults) *pdwTotalResults = 0; if (pdwReturnedResults) *pdwReturnedResults = 0; if (pdwResultsSize) *pdwResultsSize = 0)
XBOXAPI HRESULT WINAPI XOnlineQuerySelect(DWORD, ULONGLONG, DWORD, XENTITY_ID, DWORD, DWORD, const XONLINE_ATTRIBUTE *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStorageCreateServerPath(DWORD, ULONGLONG, ULONGLONG, LPCWSTR, LPWSTR, DWORD *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineStorageDownloadToMemory(DWORD, DWORD, LPCWSTR, PBYTE, DWORD, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineStorageDownloadToMemoryGetResults(XONLINETASK_HANDLE, PBYTE *, DWORD *, DWORD *, ULONGLONG *, FILETIME *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineStorageUploadFromMemory(DWORD, DWORD, LPCWSTR, FILETIME, PBYTE, DWORD, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineTeamCreate(DWORD, const XONLINE_TEAM_PROPERTIES *, const XONLINE_TEAM_MEMBER_PROPERTIES *, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineTeamCreateGetResults(XONLINETASK_HANDLE, XONLINE_TEAM *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineTeamDelete(DWORD, XUID, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineTeamEnumerate(DWORD, DWORD, const XUID *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineTeamEnumerateByUserXUID(DWORD, XUID, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineTeamEnumerateGetResults(XONLINETASK_HANDLE, DWORD *pdwTeamCount, XUID *)
    RXDK_XO_TASK_STUB(if (pdwTeamCount) *pdwTeamCount = 0)
XBOXAPI HRESULT WINAPI XOnlineTeamGetDetails(XONLINETASK_HANDLE, XUID, XONLINE_TEAM *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineTeamMemberAnswerRecruit(DWORD, XUID, XONLINE_PEER_ANSWER_TYPE, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineTeamMemberGetDetails(XONLINETASK_HANDLE, XUID, XONLINE_TEAM_MEMBER *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineTeamMemberRecruit(DWORD, XUID, XUID, const XONLINE_TEAM_MEMBER_PROPERTIES *, XONLINE_MSG_HANDLE, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineTeamMemberRemove(DWORD, XUID, XUID, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineTeamMemberSetProperties(DWORD, XUID, XUID, const XONLINE_TEAM_MEMBER_PROPERTIES *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineTeamMembersEnumerate(DWORD, XUID, DWORD, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineTeamMembersEnumerateGetResults(XONLINETASK_HANDLE, DWORD *pdwTeamMemberCount, XUID *)
    RXDK_XO_TASK_STUB(if (pdwTeamMemberCount) *pdwTeamMemberCount = 0)
XBOXAPI HRESULT WINAPI XOnlineTeamSetProperties(DWORD, XUID, const XONLINE_TEAM_PROPERTIES *, HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)

XBOXAPI HRESULT WINAPI XOnlinePresenceGetLatest(XONLINETASK_HANDLE, DWORD, DWORD, XONLINE_PRESENCE *)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlinePresenceGetTitleName(XONLINETASK_HANDLE, DWORD, DWORD, DWORD dwTitleNameSize, LPWSTR wszTitleName)
    RXDK_XO_TASK_STUB(if (wszTitleName && dwTitleNameSize) wszTitleName[0] = 0)

// --- Mutelist (voice) ---
XBOXAPI HRESULT WINAPI XOnlineMutelistStartup(HANDLE, PXONLINETASK_HANDLE phTask)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL)
XBOXAPI HRESULT WINAPI XOnlineMutelistGet(DWORD, DWORD, HANDLE, PXONLINETASK_HANDLE phTask, PXONLINE_MUTELISTUSER, DWORD *pdwNumMutelistUsers)
    RXDK_XO_TASK_STUB(if (phTask) *phTask = NULL; if (pdwNumMutelistUsers) *pdwNumMutelistUsers = 0)
XBOXAPI HRESULT WINAPI XOnlineMutelistAdd(DWORD, XUID)
    RXDK_XO_TASK_STUB((void)0)
XBOXAPI HRESULT WINAPI XOnlineMutelistRemove(DWORD, XUID)
    RXDK_XO_TASK_STUB((void)0)

}
