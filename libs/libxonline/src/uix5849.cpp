// RXDK 5849 uplift: the UIX "Drop-In UI" Live engine (uix.h / uix.lib), which the
// 5849 Live samples use for the logon / friends / players screens. The leak has
// no UIX source at all (uix.lib shipped binary-only), so this is a fresh RXDK
// implementation of the public surface. uix.lib is folded into libxonline: every
// Live sample already links libxonline, and UIX is the UI face of the same
// online stack.
//
// What is REAL here (drives the leak's actual Live client, i.e. what Insignia
// talks to):
//  - the LOGON feature: StartFeature(UIX_LOGON_FEATURE) enumerates the stored
//    accounts (XOnlineGetUsers), presents a controller-driven account picker
//    (rendered through the title's ITitleUIPlugin/ITitleFontRenderer), runs the
//    real XOnlineLogon task, pumps it in DoWork, and reports
//    UIX_EXIT_LOGON_SUCCESSFUL / _FAILED / _USER_EXIT through GetExitInfo --
//    after which XOnlineGetLogonUsers() returns the live users, exactly like
//    retail. After a successful logon the engine keeps pumping the logon task
//    every DoWork, which is what keeps the Live connection alive.
//  - UIXCreateUIPlugin / UIXCreateAudioPlugin return real (vtable) objects.
//
// What is NOT implemented (documented gaps, all TODO(5849-uix)):
//  - the FRIENDS / PLAYERS screens (StartFeature fails for them),
//  - the skin system: rendering goes straight through the plugin's font
//    renderer instead of the retail CreateObject/SetText object protocol, so
//    custom UI plugins get functional text UI rather than skinned screens,
//  - multi-user (2-4) and guest logon: the picker signs in ONE account (slot 0),
//  - passcode entry (passcodes are client-side only; accounts that require one
//    are still selectable), voicemail, notifications UI, reboot-to-dash.

#include <xtl.h>
#include <xobjbase.h>
#include <d3d8.h>
#include <xonline.h>
#include <uix.h>

#include <stdlib.h> // malloc/free
#include <string.h> // memset

// placement new (no <new> in the freestanding libxonline environment)
inline void *operator new(size_t, void *pv) { return pv; }

// ------------------------------------------------------------- feature ids ---
// Opaque feature handles; each points at its own static token so titles get
// distinct, comparable values (same scheme as libxvoice's _xhv_*_mode).
static const BYTE s_uixTokens[5] = { 0, 1, 2, 3, 4 };

extern "C" {
const UIX_FEATURE _uix_logon_feature   = (UIX_FEATURE)&s_uixTokens[0];
const UIX_FEATURE _uix_friends_feature = (UIX_FEATURE)&s_uixTokens[1];
const UIX_FEATURE _uix_players_feature = (UIX_FEATURE)&s_uixTokens[2];
const UIX_FEATURE _uix_uitest_feature  = (UIX_FEATURE)&s_uixTokens[3];

const UIX_VOICE_MAIL_ENTRY_POINT _uix_voice_mail = (UIX_VOICE_MAIL_ENTRY_POINT)&s_uixTokens[4];
}

// -------------------------------------------------------- default plugins ---
// The default UI plugin wraps the title's font renderer; the engine only pulls
// the font back out (GetFont) and draws with it. The object-protocol methods
// accept and succeed so a title driving them directly does not fail.

class CDefaultUIPlugin : public ITitleUIPlugin
{
public:
    ITitleFontRenderer *m_pFont;
    LONG                m_cRef;

    STDMETHOD_(ULONG, Release)()
    {
        LONG c = --m_cRef;
        if (c <= 0) {
            free(this);
            return 0;
        }
        return (ULONG)c;
    }
    STDMETHOD(DoWork)() { return S_OK; }
    STDMETHOD(SetPluginSupport)(IPluginSupport *) { return S_OK; }
    STDMETHOD(CreateObject)(UIX_OBJECT_TYPE, DWORD, DWORD, DWORD, DWORD *pObjectID)
    {
        if (pObjectID)
            *pObjectID = 0;
        return S_OK;
    }
    STDMETHOD(DestroyObject)(DWORD) { return S_OK; }
    STDMETHOD(DestroyScreenObjects)(DWORD) { return S_OK; }
    STDMETHOD(SetRenderTarget)(IDirect3DSurface8 *) { return S_OK; }
    STDMETHOD(RenderObject)(DWORD) { return S_OK; }
    STDMETHOD(SetText)(DWORD, DWORD, LPCWSTR, DWORD, const UIX_SKIN_ICON_INFO *) { return S_OK; }
    STDMETHOD(InsertItem)(DWORD, DWORD, DWORD *pReturnIndex)
    {
        if (pReturnIndex)
            *pReturnIndex = 0;
        return S_OK;
    }
    STDMETHOD(SetObjectState)(DWORD, DWORD, UIX_OBJSTATE_TYPE, DWORD) { return S_OK; }
    STDMETHOD(GetObjectState)(DWORD, DWORD, UIX_OBJSTATE_TYPE, DWORD *pValue)
    {
        if (pValue)
            *pValue = 0;
        return S_OK;
    }
    STDMETHOD(PassInputToObject)(DWORD, UIX_INPUT_TYPE) { return S_OK; }
    STDMETHOD(Clear)(DWORD, BOOL) { return S_OK; }
    STDMETHOD(GetFont)(ITitleFontRenderer **ppFont)
    {
        if (!ppFont)
            return E_POINTER;
        *ppFont = m_pFont;
        return m_pFont ? S_OK : E_FAIL;
    }
};

class CDefaultAudioPlugin : public ITitleAudioPlugin
{
public:
    LONG m_cRef;

    STDMETHOD_(ULONG, Release)()
    {
        LONG c = --m_cRef;
        if (c <= 0) {
            free(this);
            return 0;
        }
        return (ULONG)c;
    }
    STDMETHOD(PlaySound)(LPCSTR) { return S_OK; } // no skin sound banks; silent
    STDMETHOD(DoWork)() { return S_OK; }
};

// ----------------------------------------------------------------- engine ---

enum RXDK_UIX_STATE
{
    UIXST_IDLE = 0,     // no feature active
    UIXST_PICKING,      // logon: account picker on screen
    UIXST_CONNECTING,   // logon: XOnlineLogon task running
    UIXST_EXIT_PENDING, // feature finished; waiting for GetExitInfo
    UIXST_LOGGED_ON,    // logon done + reported; keep pumping the logon task
};

struct RXDK_UIX_ENGINE
{
    LONG                 lRefCount;
    ITitleUIPlugin      *pUIPlugin;
    ITitleAudioPlugin   *pAudioPlugin;
    DWORD                dwEnabledFeatures; // bitmask by token index

    RXDK_UIX_STATE       state;

    // latest controller input (SetInput), NULL slots empty
    XINPUT_STATE         input[XONLINE_MAX_LOGON_USERS];
    BOOL                 fInputValid[XONLINE_MAX_LOGON_USERS];
    DWORD                dwPrevPressed[XONLINE_MAX_LOGON_USERS]; // edge detection

    // logon feature
    UIX_LOGON_PARAMS     logonParams;
    XONLINE_USER         accounts[XONLINE_MAX_STORED_ONLINE_USERS];
    DWORD                cAccounts;
    DWORD                iSelected;
    DWORD                dwPickPort;   // controller that confirmed
    XONLINETASK_HANDLE   hLogonTask;

    // exit reporting
    BOOL                 fExitPending;
    UIX_EXIT_INFO        exitInfo;
    UIX_LOGON_EXIT_DATA  logonExitData;
};

static RXDK_UIX_ENGINE *Eng(LiveEngine *pThis)
{
    return (RXDK_UIX_ENGINE *)pThis;
}

// virtual UIX keys decoded from XINPUT (edge-triggered)
#define UIXKEY_A     0x01
#define UIXKEY_B     0x02
#define UIXKEY_UP    0x04
#define UIXKEY_DOWN  0x08

#define UIX_ANALOG_PRESS_THRESHOLD 0x20
#define UIX_THUMB_DEADZONE         20000

static DWORD DecodePressed(const XINPUT_STATE *pState)
{
    DWORD k = 0;
    if (pState->Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] > UIX_ANALOG_PRESS_THRESHOLD)
        k |= UIXKEY_A;
    if (pState->Gamepad.bAnalogButtons[XINPUT_GAMEPAD_B] > UIX_ANALOG_PRESS_THRESHOLD)
        k |= UIXKEY_B;
    if ((pState->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) ||
        pState->Gamepad.sThumbLY > UIX_THUMB_DEADZONE)
        k |= UIXKEY_UP;
    if ((pState->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ||
        pState->Gamepad.sThumbLY < -UIX_THUMB_DEADZONE)
        k |= UIXKEY_DOWN;
    return k;
}

static void SetExit(RXDK_UIX_ENGINE *e, UIX_EXIT_CODE_TYPE code, HRESULT hr, PVOID pData)
{
    e->exitInfo.FeatureID = _uix_logon_feature;
    e->exitInfo.ExitCode  = code;
    e->exitInfo.hr        = hr;
    e->exitInfo.pExitData = pData;
    e->fExitPending       = TRUE;
    e->state              = UIXST_EXIT_PENDING;
}

static void StartLogonTask(RXDK_UIX_ENGINE *e)
{
    // Sign in ONE account in slot 0 (multi-user/guest logon: TODO(5849-uix)).
    XONLINE_USER logonUsers[XONLINE_MAX_LOGON_USERS];
    memset(logonUsers, 0, sizeof(logonUsers));
    logonUsers[0] = e->accounts[e->iSelected];

    DWORD cServices = 0;
    while (cServices < UIX_MAX_LOGON_SERVICES && e->logonParams.LogonServiceIDs[cServices])
        cServices++;

    HRESULT hr = XOnlineLogon(logonUsers, e->logonParams.LogonServiceIDs, cServices,
                              NULL, &e->hLogonTask);
    if (FAILED(hr)) {
        e->hLogonTask = NULL;
        SetExit(e, UIX_EXIT_LOGON_FAILED, hr, NULL);
        return;
    }
    e->state = UIXST_CONNECTING;
}

// widen an ASCII gamertag for the font renderer
static void WidenTag(const CHAR *src, WCHAR *dst, size_t cch)
{
    size_t i = 0;
    for (; i + 1 < cch && src[i]; i++)
        dst[i] = (WCHAR)(unsigned char)src[i];
    dst[i] = 0;
}

extern "C" {

XBOXAPI HRESULT WINAPI UIXCreateLiveEngine(LPCSTR pSkinFileName, DWORD LanguageID,
                                           ILiveEngine **ppEngine)
{
    // The skin file is not loaded: rendering goes through the title's font
    // renderer (see file header). Language is ignored (English UI strings).
    (void)pSkinFileName;
    (void)LanguageID;
    if (!ppEngine)
        return E_POINTER;
    RXDK_UIX_ENGINE *e = (RXDK_UIX_ENGINE *)malloc(sizeof(RXDK_UIX_ENGINE));
    if (!e) {
        *ppEngine = NULL;
        return E_OUTOFMEMORY;
    }
    memset(e, 0, sizeof(*e));
    e->lRefCount = 1;
    e->state     = UIXST_IDLE;
    *ppEngine = (ILiveEngine *)e;
    return S_OK;
}

XBOXAPI HRESULT WINAPI UIXCreateUIPlugin(ITitleFontRenderer *pFont, ITitleUIPlugin **ppUIPlugin)
{
    if (!ppUIPlugin)
        return E_POINTER;
    CDefaultUIPlugin *p = (CDefaultUIPlugin *)malloc(sizeof(CDefaultUIPlugin));
    if (!p) {
        *ppUIPlugin = NULL;
        return E_OUTOFMEMORY;
    }
    // placement-construct so the vtable pointer is installed
    p = new (p) CDefaultUIPlugin;
    p->m_pFont = pFont;
    p->m_cRef  = 1;
    *ppUIPlugin = p;
    return S_OK;
}

XBOXAPI HRESULT WINAPI UIXCreateAudioPlugin(VOID *pXactEngine, VOID *pXactSoundBank,
                                            ITitleAudioPlugin **ppAudioPlugin)
{
    // The retail plugin plays skin sounds through the title's XACT engine; ours
    // is a functional no-op object (no skin sound banks are ever loaded).
    (void)pXactEngine;
    (void)pXactSoundBank;
    if (!ppAudioPlugin)
        return E_POINTER;
    CDefaultAudioPlugin *p = (CDefaultAudioPlugin *)malloc(sizeof(CDefaultAudioPlugin));
    if (!p) {
        *ppAudioPlugin = NULL;
        return E_OUTOFMEMORY;
    }
    p = new (p) CDefaultAudioPlugin;
    p->m_cRef = 1;
    *ppAudioPlugin = p;
    return S_OK;
}

ULONG WINAPI LiveEngine_AddRef(LiveEngine *pThis)
{
    return (ULONG)(++Eng(pThis)->lRefCount);
}

ULONG WINAPI LiveEngine_Release(LiveEngine *pThis)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    LONG cRef = --e->lRefCount;
    if (cRef <= 0) {
        if (e->hLogonTask)
            XOnlineTaskClose(e->hLogonTask);
        free(e);
        return 0;
    }
    return (ULONG)cRef;
}

HRESULT WINAPI LiveEngine_SetUIPlugin(LiveEngine *pThis, ITitleUIPlugin *pUIPlugin)
{
    Eng(pThis)->pUIPlugin = pUIPlugin;
    return S_OK;
}

HRESULT WINAPI LiveEngine_SetAudioPlugin(LiveEngine *pThis, ITitleAudioPlugin *pAudioPlugin)
{
    Eng(pThis)->pAudioPlugin = pAudioPlugin;
    return S_OK;
}

HRESULT WINAPI LiveEngine_EnableFeature(LiveEngine *pThis, UIX_FEATURE FeatureID)
{
    const BYTE *p = (const BYTE *)FeatureID;
    if (p < s_uixTokens || p >= s_uixTokens + 4)
        return E_INVALIDARG;
    Eng(pThis)->dwEnabledFeatures |= (1u << (p - s_uixTokens));
    return S_OK;
}

HRESULT WINAPI LiveEngine_StartFeature(LiveEngine *pThis, UIX_FEATURE FeatureID,
                                       const VOID *pFeatureParams)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);

    // Only the logon feature is implemented; friends/players need the full
    // drop-in UI. TODO(5849-uix).
    if (FeatureID != _uix_logon_feature)
        return E_FAIL;
    if (!(e->dwEnabledFeatures & 0x1))
        return E_FAIL; // not enabled
    if (e->state == UIXST_PICKING || e->state == UIXST_CONNECTING)
        return E_FAIL; // already running

    const UIX_LOGON_PARAMS *pParams = (const UIX_LOGON_PARAMS *)pFeatureParams;
    if (!pParams || pParams->StructSize != sizeof(UIX_LOGON_PARAMS))
        return E_INVALIDARG;
    if (pParams->LogonType == UIX_LOGON_TYPE_RETRIEVED_STATE ||
        pParams->LogonType == UIX_LOGON_TYPE_RETRIEVED_GAME_INVITE)
        return E_FAIL; // saved-state/invite restore not implemented

    // leaving a previous logon: drop its task
    if (e->hLogonTask) {
        XOnlineTaskClose(e->hLogonTask);
        e->hLogonTask = NULL;
    }

    e->logonParams = *pParams;
    memset(&e->logonExitData, 0xFF, sizeof(e->logonExitData)); // ports default -1
    e->fExitPending = FALSE;

    // Enumerate the stored accounts (hard disk + MUs).
    e->cAccounts = 0;
    HRESULT hr = XOnlineGetUsers(e->accounts, &e->cAccounts);
    if (FAILED(hr)) {
        SetExit(e, UIX_EXIT_LOGON_FAILED, hr, NULL);
        return S_OK; // feature started; the failure is reported via exit info
    }
    if (e->cAccounts == 0) {
        // No accounts on this box: retail sends the player to account signup.
        SetExit(e, UIX_EXIT_LOGON_FAILED, XONLINE_E_USER_NOT_PRESENT, NULL);
        return S_OK;
    }

    e->iSelected  = 0;
    e->dwPickPort = 0;
    memset(e->dwPrevPressed, 0, sizeof(e->dwPrevPressed));

    if (pParams->LogonType == UIX_LOGON_TYPE_SILENT) {
        // no UI: first stored account
        StartLogonTask(e);
    } else {
        e->state = UIXST_PICKING;
    }
    return S_OK;
}

HRESULT WINAPI LiveEngine_SetInput(LiveEngine *pThis, DWORD Port, const XINPUT_STATE *pInputState)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    if (Port >= XONLINE_MAX_LOGON_USERS)
        return E_INVALIDARG;
    if (pInputState) {
        e->input[Port]       = *pInputState;
        e->fInputValid[Port] = TRUE;
    } else {
        e->fInputValid[Port] = FALSE;
        e->dwPrevPressed[Port] = 0;
    }
    return S_OK;
}

HRESULT WINAPI LiveEngine_ClearInput(LiveEngine *pThis)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    memset(e->fInputValid, 0, sizeof(e->fInputValid));
    memset(e->dwPrevPressed, 0, sizeof(e->dwPrevPressed));
    return S_OK;
}

HRESULT WINAPI LiveEngine_DoWork(LiveEngine *pThis, DWORD *pDoWorkFlags)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    DWORD flags = 0;

    if (e->pUIPlugin && e->state != UIXST_IDLE)
        e->pUIPlugin->DoWork();
    if (e->pAudioPlugin)
        e->pAudioPlugin->DoWork();

    switch (e->state) {

    case UIXST_PICKING:
    {
        flags = UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT;
        for (DWORD port = 0; port < XONLINE_MAX_LOGON_USERS; port++) {
            if (!e->fInputValid[port])
                continue;
            DWORD pressed = DecodePressed(&e->input[port]);
            DWORD edges   = pressed & ~e->dwPrevPressed[port];
            e->dwPrevPressed[port] = pressed;

            if (edges & UIXKEY_UP) {
                e->iSelected = e->iSelected ? e->iSelected - 1 : e->cAccounts - 1;
            } else if (edges & UIXKEY_DOWN) {
                e->iSelected = (e->iSelected + 1) % e->cAccounts;
            } else if (edges & UIXKEY_A) {
                // Passcode accounts: passcodes are client-side only; entry UI is
                // TODO(5849-uix) -- the account signs in without the prompt.
                e->dwPickPort = port;
                e->logonExitData.pMappedControllers[0] = port;
                StartLogonTask(e);
                break;
            } else if (edges & UIXKEY_B) {
                SetExit(e, UIX_EXIT_LOGON_USER_EXIT, S_OK, NULL);
                break;
            }
        }
        if (e->state == UIXST_EXIT_PENDING)
            flags = UIX_DOWORK_FEATURE_EXIT;
        else if (e->state == UIXST_CONNECTING)
            flags = UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT;
        break;
    }

    case UIXST_CONNECTING:
    {
        flags = UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT;

        // allow the player to abort the connection attempt
        for (DWORD port = 0; port < XONLINE_MAX_LOGON_USERS; port++) {
            if (!e->fInputValid[port])
                continue;
            DWORD pressed = DecodePressed(&e->input[port]);
            DWORD edges   = pressed & ~e->dwPrevPressed[port];
            e->dwPrevPressed[port] = pressed;
            if (edges & UIXKEY_B) {
                XOnlineTaskClose(e->hLogonTask);
                e->hLogonTask = NULL;
                SetExit(e, UIX_EXIT_LOGON_USER_EXIT, S_OK, NULL);
            }
        }
        if (e->state != UIXST_CONNECTING) {
            flags = UIX_DOWORK_FEATURE_EXIT;
            break;
        }

        HRESULT hr = XOnlineTaskContinue(e->hLogonTask);
        if (hr == XONLINETASK_S_RUNNING || hr == XONLINETASK_S_RUNNING_IDLE)
            break;
        if (hr == XONLINE_S_LOGON_CONNECTION_ESTABLISHED || SUCCEEDED(hr)) {
            // per-user results live in XOnlineGetLogonUsers()[i].hr
            SetExit(e, UIX_EXIT_LOGON_SUCCESSFUL, hr, &e->logonExitData);
        } else {
            XOnlineTaskClose(e->hLogonTask);
            e->hLogonTask = NULL;
            SetExit(e, UIX_EXIT_LOGON_FAILED, hr, NULL);
        }
        flags = UIX_DOWORK_FEATURE_EXIT;
        break;
    }

    case UIXST_EXIT_PENDING:
        flags = UIX_DOWORK_FEATURE_EXIT;
        break;

    case UIXST_LOGGED_ON:
        // keep the Live connection alive; a mid-session drop surfaces through
        // the title's own service calls. TODO(5849-uix): notifications UI.
        if (e->hLogonTask)
            XOnlineTaskContinue(e->hLogonTask);
        break;

    case UIXST_IDLE:
    default:
        break;
    }

    if (pDoWorkFlags)
        *pDoWorkFlags = flags;
    return S_OK;
}

HRESULT WINAPI LiveEngine_Render(LiveEngine *pThis, IDirect3DSurface8 *pSurface)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    if (e->state != UIXST_PICKING && e->state != UIXST_CONNECTING)
        return S_OK;
    if (!e->pUIPlugin)
        return S_OK;

    e->pUIPlugin->SetRenderTarget(pSurface);

    ITitleFontRenderer *pFont = NULL;
    if (FAILED(e->pUIPlugin->GetFont(&pFont)) || !pFont)
        return S_OK;

    const DWORD X = 96, W = 448;
    DWORD y = 96;

    pFont->SetHeight(28);
    pFont->SetColor(0xFFFFFFFF);
    pFont->DrawText(L"Xbox Live", X, y, W);
    y += 44;

    pFont->SetHeight(20);

    if (e->state == UIXST_CONNECTING) {
        WCHAR wszTag[XONLINE_GAMERTAG_SIZE];
        WidenTag(e->accounts[e->iSelected].szGamertag, wszTag, XONLINE_GAMERTAG_SIZE);
        pFont->SetColor(0xFFC0C0C0);
        pFont->DrawText(L"Connecting to Xbox Live as", X, y, W);
        y += 28;
        pFont->SetColor(0xFFFFFFFF);
        pFont->DrawText(wszTag, X + 16, y, W - 16);
        y += 40;
        pFont->SetColor(0xFF808080);
        pFont->DrawText(L"B Cancel", X, y, W);
        return S_OK;
    }

    pFont->SetColor(0xFFC0C0C0);
    pFont->DrawText(L"Select an account", X, y, W);
    y += 32;

    // window the list around the selection (8 rows)
    const DWORD ROWS = 8;
    DWORD first = 0;
    if (e->cAccounts > ROWS && e->iSelected >= ROWS / 2) {
        first = e->iSelected - ROWS / 2;
        if (first + ROWS > e->cAccounts)
            first = e->cAccounts - ROWS;
    }
    for (DWORD i = first; i < e->cAccounts && i < first + ROWS; i++) {
        WCHAR wszTag[XONLINE_GAMERTAG_SIZE + 2];
        WidenTag(e->accounts[i].szGamertag, wszTag, XONLINE_GAMERTAG_SIZE);
        if (i == e->iSelected) {
            pFont->SetColor(0xFF00FF00);
            pFont->DrawText(L">", X, y, 16);
        } else {
            pFont->SetColor(0xFFFFFFFF);
        }
        pFont->DrawText(wszTag, X + 24, y, W - 24);
        y += 26;
    }

    y += 12;
    pFont->SetColor(0xFF808080);
    pFont->DrawText(L"A Select    B Cancel", X, y, W);
    return S_OK;
}

// No feature ever started can have exited until the logon reports; after the
// title reads the exit info the engine goes back to pumping (LOGGED_ON) or idle.
HRESULT WINAPI LiveEngine_GetExitInfo(LiveEngine *pThis, UIX_EXIT_INFO *pExitInfo)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    if (!pExitInfo)
        return E_POINTER;
    if (!e->fExitPending) {
        memset(pExitInfo, 0, sizeof(*pExitInfo));
        return E_FAIL;
    }
    *pExitInfo = e->exitInfo;
    e->fExitPending = FALSE;
    e->state = (e->exitInfo.ExitCode == UIX_EXIT_LOGON_SUCCESSFUL) ? UIXST_LOGGED_ON
                                                                   : UIXST_IDLE;
    return S_OK;
}

HRESULT WINAPI LiveEngine_EndFeature(LiveEngine *pThis)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    if (e->state == UIXST_PICKING || e->state == UIXST_CONNECTING) {
        if (e->hLogonTask) {
            XOnlineTaskClose(e->hLogonTask);
            e->hLogonTask = NULL;
        }
        e->state = UIXST_IDLE;
        e->fExitPending = FALSE;
    }
    return S_OK;
}

HRESULT WINAPI LiveEngine_LogOff(LiveEngine *pThis)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    if (e->hLogonTask) {
        XOnlineTaskClose(e->hLogonTask);
        e->hLogonTask = NULL;
    }
    if (e->state == UIXST_LOGGED_ON)
        e->state = UIXST_IDLE;
    return S_OK;
}

HRESULT WINAPI LiveEngine_GetFriendsForUser(LiveEngine *pThis, DWORD Port,
                                            ILiveFriendsList **ppFriendsList)
{
    // needs the friends feature. TODO(5849-uix).
    (void)pThis;
    (void)Port;
    if (ppFriendsList)
        *ppFriendsList = NULL;
    return E_FAIL;
}

HRESULT WINAPI LiveEngine_NotificationSetState(LiveEngine *pThis, DWORD Port, DWORD StateFlags,
                                               XNKID SessionID, DWORD StateDataSize,
                                               const PVOID pStateData)
{
    // forward to the real Live client so presence/join-ability is published
    (void)pThis;
    return XOnlineNotificationSetState(Port, StateFlags, SessionID, StateDataSize,
                                       (const BYTE *)pStateData);
}

HRESULT WINAPI LiveEngine_GetNotifications(LiveEngine *pThis, DWORD Port,
                                           UIX_NOTIFICATION_PURPOSE Purpose, DWORD *pNotifications)
{
    (void)pThis;
    (void)Port;
    (void)Purpose;
    if (!pNotifications)
        return E_POINTER;
    *pNotifications = 0; // notifications UI: TODO(5849-uix)
    return S_OK;
}

HRESULT WINAPI LiveEngine_SetProperty(LiveEngine *pThis, UIX_PROPERTY_TYPE Property, DWORD Value)
{
    (void)pThis;
    (void)Property;
    (void)Value;
    return S_OK;
}

HRESULT WINAPI LiveEngine_GetProperty(LiveEngine *pThis, UIX_PROPERTY_TYPE Property, DWORD *pValue)
{
    (void)pThis;
    (void)Property;
    if (!pValue)
        return E_POINTER;
    *pValue = 0;
    return S_OK;
}

HRESULT WINAPI LiveEngine_Reboot(LiveEngine *pThis, DWORD Context)
{
    (void)pThis;
    (void)Context;
    return E_FAIL; // dashboard reboot contexts need the real UIX/dash handshake
}

HRESULT WINAPI LiveEngine_GetFeatureInterface(LiveEngine *pThis, UIX_FEATURE FeatureID,
                                              const VOID *pParam, VOID **ppFeatureInterface)
{
    (void)pThis;
    (void)FeatureID;
    (void)pParam;
    if (ppFeatureInterface)
        *ppFeatureInterface = NULL;
    return E_FAIL;
}

HRESULT WINAPI LiveEngine_GetSelectionInfo(LiveEngine *pThis, UIX_SELECTION_INFO *pSelectionInfo)
{
    (void)pThis;
    if (pSelectionInfo)
        memset(pSelectionInfo, 0, sizeof(*pSelectionInfo));
    return E_FAIL; // friends/players selection needs those features
}

HRESULT WINAPI LiveEngine_UseVoiceMail(LiveEngine *pThis, UIX_VOICE_MAIL_ENTRY_POINT VoiceMailEntryPoint)
{
    (void)pThis;
    (void)VoiceMailEntryPoint;
    return E_FAIL; // voicemail needs the WMAVoice codec + UIX voice-mail UI
}

// ---------------------------------------------------------------- plugin ---
// PluginSupport: the skin-resource access interface UIX hands to a title's
// custom UI plugin. No skin is ever loaded (see file header), so every getter
// fails cleanly; custom plugins fall back to their own resources.

ULONG WINAPI PluginSupport_AddRef(PluginSupport *pThis)
{
    (void)pThis;
    return 1;
}

ULONG WINAPI PluginSupport_Release(PluginSupport *pThis)
{
    (void)pThis;
    return 0;
}

HRESULT WINAPI PluginSupport_GetString(PluginSupport *pThis, DWORD StringResID, LPCWSTR *ppString)
{
    (void)pThis;
    (void)StringResID;
    if (ppString)
        *ppString = NULL;
    return E_FAIL;
}

HRESULT WINAPI PluginSupport_GetLayout(PluginSupport *pThis, DWORD ScreenResID, DWORD ObjectResID,
                                       UIX_SKIN_LAYOUT_INFO **ppLayout)
{
    (void)pThis;
    (void)ScreenResID;
    (void)ObjectResID;
    if (ppLayout)
        *ppLayout = NULL;
    return E_FAIL;
}

HRESULT WINAPI PluginSupport_GetImage(PluginSupport *pThis, DWORD ImageResID, IDirect3DTexture8 **ppTexture)
{
    (void)pThis;
    (void)ImageResID;
    if (ppTexture)
        *ppTexture = NULL;
    return E_FAIL;
}

HRESULT WINAPI PluginSupport_GetScreenImage(PluginSupport *pThis, DWORD ScreenResID, DWORD ImageResID,
                                            IDirect3DTexture8 **ppTexture)
{
    (void)pThis;
    (void)ScreenResID;
    (void)ImageResID;
    if (ppTexture)
        *ppTexture = NULL;
    return E_FAIL;
}

HRESULT WINAPI PluginSupport_GetWordLength(PluginSupport *pThis, LPCWSTR pString, DWORD *pWordLength)
{
    (void)pThis;
    (void)pString;
    if (pWordLength)
        *pWordLength = 0;
    return E_FAIL;
}

} // extern "C"
