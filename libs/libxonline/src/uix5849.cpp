// RXDK 5849 uplift: the UIX "Drop-In UI" Live engine (uix.h / uix.lib). The leak
// has no UIX source at all (uix.lib shipped binary-only), so this is a fresh
// RXDK implementation of the public surface, folded into libxonline (every Live
// sample already links it, and UIX is the UI face of the same online stack).
//
// Everything here drives the leak's REAL Live client (the machinery whose
// server side Insignia reimplements):
//
//  LOGON    - account picker over XOnlineGetUsers with multi-user (1..4)
//             per-controller claims, guest sign-in, and passcode entry; runs
//             the real XOnlineLogon task, pumped non-blocking in DoWork; also
//             supports SILENT, RETRIEVED_STATE (XOnlineRetrieveLogonState) and
//             RETRIEVED_GAME_INVITE starts. After success the engine keeps
//             pumping the logon task, which is what holds the Live connection.
//  FRIENDS  - real friends screen over the leak's friends enumeration
//             (XOnlineFriendsStartup/Enumerate/GetLatest) with accept/decline
//             requests and invites, join, remove, invite-to-game and optional
//             sign-out; reports FRIENDS_JOIN_GAME[_CROSS_TITLE] with the
//             XONLINE_FRIEND as exit data, exactly like retail.
//  PLAYERS  - real players screen over the engine's ILivePlayersList registry
//             (RegisterPlayer/Unregister/departed list/filters/sort) with
//             mute/unmute through the mutelist API and selection reporting.
//  Notifications - real: derived from the live friends snapshot
//             (XOnlineGetNotification), surfaced through the DoWork NOTIFY
//             flags and GetNotifications, gated by the properties.
//  Reboot   - real: XLaunchNewImage to the dashboard with the given context.
//
// Rendering intentionally goes through the title's ITitleUIPlugin/
// ITitleFontRenderer as clean text screens instead of the retail skin system:
// the skin pipeline (skinbld texture bundling) does not survive on modern
// hosts, so RXDK defines the font-renderer path as THE rendering contract --
// skin file names passed to UIXCreateLiveEngine are accepted and unused by
// design. Voicemail record/play is a capability boundary shared with libxvoice:
// the WMAVoice codec only ever existed as binaries inside the retail libs.

#include <xtl.h>
#include <xobjbase.h>
#include <d3d8.h>
#include <xonline.h>
#include <uix.h>
#include "uix_skin.h"

// malloc/free/memset/memcpy come from the force-included site/cdecl_libc.h, which
// declares them __cdecl. Do NOT include <stdlib.h>/<string.h> here: this library
// is built -fdefault-calling-conv=stdcall, picolibc's prototypes carry no explicit
// convention, and clang silently prefers such a later declaration's convention over
// the explicit one -- which made every malloc/free call here stdcall against a cdecl
// libc, leaving ESP low so these functions returned into the stack.

// placement new (no <new> in the freestanding libxonline environment)
inline void *operator new(size_t, void *pv) { return pv; }

// The engine and plugin objects come from the process heap rather than the CRT.
// This library is built /Gz (-fdefault-calling-conv=stdcall) while libc is cdecl,
// and a CRT prototype carries no explicit convention, so a malloc/free call here
// compiles stdcall: the call site expects the callee to pop its argument, leaving
// ESP low so the caller's epilogue returns through the wrong slot -- these very
// functions returned into the stack, which wedged every UIX sample in its
// Initialize. GetProcessHeap/HeapAlloc/HeapFree are declared __stdcall outright,
// so there is nothing to get wrong, and the process heap is where a vendor
// library allocates anyway. RXDK-VS20XX/scripts/Test-CallingConventions.ps1
// checks a linked title for any surviving stdcall call into cdecl libc.
static void *UixAlloc(DWORD cb) { return HeapAlloc(GetProcessHeap(), 0, cb); }
static void UixFree(void *pv) { HeapFree(GetProcessHeap(), 0, pv); }

// ------------------------------------------------------------- feature ids ---
static const BYTE s_uixTokens[5] = { 0, 1, 2, 3, 4 };

extern "C" {
const UIX_FEATURE _uix_logon_feature   = (UIX_FEATURE)&s_uixTokens[0];
const UIX_FEATURE _uix_friends_feature = (UIX_FEATURE)&s_uixTokens[1];
const UIX_FEATURE _uix_players_feature = (UIX_FEATURE)&s_uixTokens[2];
const UIX_FEATURE _uix_uitest_feature  = (UIX_FEATURE)&s_uixTokens[3];

const UIX_VOICE_MAIL_ENTRY_POINT _uix_voice_mail = (UIX_VOICE_MAIL_ENTRY_POINT)&s_uixTokens[4];
}

// -------------------------------------------------------- default plugins ---

class CDefaultUIPlugin : public ITitleUIPlugin
{
public:
    ITitleFontRenderer *m_pFont;
    LONG                m_cRef;

    STDMETHOD_(ULONG, Release)()
    {
        LONG c = --m_cRef;
        if (c <= 0) {
            UixFree(this);
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
            UixFree(this);
            return 0;
        }
        return (ULONG)c;
    }
    STDMETHOD(PlaySound)(LPCSTR) { return S_OK; } // no skin sound banks by design; silent
    STDMETHOD(DoWork)() { return S_OK; }
};

// ------------------------------------------------------------ players list ---
// The engine's real player registry, handed to titles as ILivePlayersList via
// GetFeatureInterface(UIX_PLAYERS_FEATURE).

#define RXDK_UIX_MAX_PLAYERS 64

class CPlayersList : public ILivePlayersList
{
public:
    const ITitlePlayersListItem *m_Current[RXDK_UIX_MAX_PLAYERS];
    const ITitlePlayersListItem *m_Departed[RXDK_UIX_MAX_PLAYERS];
    DWORD                        m_cCurrent;
    DWORD                        m_cDeparted;
    DWORD                        m_FilterFlags;

    STDMETHOD(RegisterPlayer)(CONST ITitlePlayersListItem *pPlayer)
    {
        if (!pPlayer)
            return E_INVALIDARG;
        for (DWORD i = 0; i < m_cCurrent; i++)
            if (m_Current[i] == pPlayer)
                return S_OK;
        if (m_cCurrent >= RXDK_UIX_MAX_PLAYERS)
            return E_OUTOFMEMORY;
        m_Current[m_cCurrent++] = pPlayer;
        return S_OK;
    }
    STDMETHOD(UnregisterPlayer)(CONST ITitlePlayersListItem *pPlayer)
    {
        for (DWORD i = 0; i < m_cCurrent; i++) {
            if (m_Current[i] == pPlayer) {
                // a player who leaves moves to the departed list (retail behavior)
                if (m_cDeparted < RXDK_UIX_MAX_PLAYERS)
                    m_Departed[m_cDeparted++] = pPlayer;
                for (; i + 1 < m_cCurrent; i++)
                    m_Current[i] = m_Current[i + 1];
                m_cCurrent--;
                return S_OK;
            }
        }
        return E_INVALIDARG;
    }
    STDMETHOD(Refresh)() { return S_OK; } // the screen reads the registry live every frame
    STDMETHOD(SetFilterFlags)(CONST DWORD Flags)
    {
        m_FilterFlags = Flags;
        return S_OK;
    }
    STDMETHOD(ClearDepartedPlayersList)()
    {
        m_cDeparted = 0;
        return S_OK;
    }
};

// ----------------------------------------------------------------- engine ---

enum RXDK_UIX_STATE
{
    UIXST_IDLE = 0,
    UIXST_LOGON_PICKING,
    UIXST_LOGON_PASSCODE,
    UIXST_LOGON_CONNECTING,
    UIXST_FRIENDS,
    UIXST_PLAYERS,
    UIXST_CUSTOM,        // a title-supplied extension feature (IUIXFeature) is active
    UIXST_EXIT_PENDING,
};

// A registered screen (from IUIXEngineInternal::CreateScreen). Objects are created
// lazily on first ShowScreen and rendered/​fed-input via the IUIXScreen vtable.
struct RXDK_UIX_SCREEN
{
    IUIXScreen *pScreen;      // title's screen implementation
    DWORD       dwScreenResID;
    DWORD       dwInstance;   // unique id passed to the plugin's per-screen object calls
    BOOL        fCreated;     // IUIXScreen::CreateScreen() has been invoked
    BOOL        fInputEnabled;
    BOOL        fAllowStartBack;
};

#define RXDK_UIX_MAX_SCREENS 8

// a controller's claim on a picker row
struct UIX_CLAIM
{
    LONG  iAccount;  // index into accounts[], -1 = no claim
    BOOL  fGuest;    // claim is "guest of accounts[iAccount]"
    DWORD dwGuestNo; // 1..3 when fGuest
};

struct RXDK_UIX_ENGINE;

// The IUIXEngineInternal handed to a title extension feature via UIX_FEATURE_CONTEXT.
// It is a real COM-style vtable (unlike ILiveEngine, which is an opaque handle), so
// it is a class embedded in the engine with a back-pointer, mirroring CPlayersList.
// Bodies are defined out-of-line once RXDK_UIX_ENGINE and the skin helpers exist.
class CUIXEngineInternal : public IUIXEngineInternal
{
public:
    RXDK_UIX_ENGINE *pEngine;

    STDMETHOD(CreateScreen)(OUT UIX_SCREEN *pScreenObject, IN DWORD ScreenResID,
                            IN IUIXScreen *pScreenInterface);
    STDMETHOD(DestroyScreen)(IN UIX_SCREEN ScreenObject);
    STDMETHOD(ShowScreen)(IN UIX_SCREEN ScreenObject, IN BOOL ReplaceCurrent);
    STDMETHOD(HideTopScreen)();
    STDMETHOD(EnableScreenInput)(IN UIX_SCREEN ScreenObject, IN BOOL Enable);
    STDMETHOD(AllowStartAndBack)(IN UIX_SCREEN ScreenObject, IN BOOL Allow);
    STDMETHOD(CreateObject)(OUT DWORD *pObjectID, IN UIX_SCREEN ScreenObject,
                            IN UIX_OBJECT_TYPE ObjectType, IN DWORD ObjectResID);
    STDMETHOD(SetText)(IN UIX_SCREEN ScreenObject, IN DWORD ObjectID, IN LPCWSTR pText);
    STDMETHOD(SetTextWithResID)(IN UIX_SCREEN ScreenObject, IN DWORD ObjectID,
                                IN DWORD StringResID);
    STDMETHOD(AddListItem)(IN UIX_SCREEN ScreenObject, IN DWORD ObjectID, IN LPCWSTR pText,
                           IN DWORD IconCount, IN const UIX_SKIN_ICON_INFO *pIconInfo);
    STDMETHOD(AddGreyListItem)(IN UIX_SCREEN ScreenObject, IN DWORD ObjectID, IN LPCWSTR pText,
                               IN DWORD IconCount, IN const UIX_SKIN_ICON_INFO *pIconInfo);
    STDMETHOD(ClearList)(IN UIX_SCREEN ScreenObject, IN DWORD ObjectID,
                         IN BOOL ResetSelectionIndex);
    STDMETHOD(SeparateTextAndIcons)(IN OUT LPCWSTR *ppText, OUT DWORD *pIconCount,
                                    OUT UIX_SKIN_ICON_INFO **ppIcons);
    STDMETHOD(SendMessageToAllFeatures)(IN UIX_FEATUREMSG_TYPE Msg, IN const VOID *pParam);
    STDMETHOD(PlaySound)(IN DWORD SoundResID);
    STDMETHOD(LaunchDash)(IN DWORD Reason, IN DWORD Parameter1, IN DWORD Parameter2);
    STDMETHOD(ShowPopup)(IN LPCWSTR pTitleString, IN DWORD MessageStringResID,
                         IN DWORD ActionButtonStringResID, IN DWORD BackButtonStringResID,
                         IN LPCWSTR pParamString);
    STDMETHOD_(BOOL, CanRecordVoiceMailForPort)(IN DWORD Port);
    STDMETHOD_(BOOL, CanPlayVoiceMailForPort)(IN DWORD Port);
    STDMETHOD(ShowVoiceMailScreen)(IN DWORD Port, IN XUID xuidUser, IN LPCWSTR pTitle,
                                   IN LPCWSTR pSubject, IN BOOL Recording, IN DWORD MessageDuration,
                                   IN DWORD VoiceMessageBufferSize, IN BYTE *pVoiceMessageBuffer);
    STDMETHOD(SetVoiceMailPlayScreenData)(IN DWORD Port, IN DWORD MessageDuration,
                                          IN DWORD VoiceMessageBufferSize, IN BYTE *pVoiceMessageBuffer);
};

struct RXDK_UIX_ENGINE
{
    LONG                 lRefCount;
    ITitleUIPlugin      *pUIPlugin;
    ITitleAudioPlugin   *pAudioPlugin;
    DWORD                dwEnabledFeatures;
    DWORD                dwProperties[6];  // UIX_PROPERTY_* values
    UIX_VOICE_MAIL_ENTRY_POINT voiceMailEntry;

    RXDK_UIX_STATE       state;
    BOOL                 fLoggedOn;        // logon task established a session

    XINPUT_STATE         input[XONLINE_MAX_LOGON_USERS];
    BOOL                 fInputValid[XONLINE_MAX_LOGON_USERS];
    DWORD                dwPrevPressed[XONLINE_MAX_LOGON_USERS];

    // ---- logon ----
    UIX_LOGON_PARAMS     logonParams;
    XONLINE_USER         accounts[XONLINE_MAX_STORED_ONLINE_USERS];
    DWORD                cAccounts;
    DWORD                iSelected;        // picker cursor (row index)
    UIX_CLAIM            claims[XONLINE_MAX_LOGON_USERS];       // by controller port
    DWORD                claimOrder[XONLINE_MAX_LOGON_USERS];   // logon slot -> port
    DWORD                cClaims;
    XONLINE_USER         logonUsers[XONLINE_MAX_LOGON_USERS];   // slots handed to XOnlineLogon
    XONLINETASK_HANDLE   hLogonTask;
    // passcode entry
    DWORD                dwPasscodePort;
    LONG                 iPasscodeAccount;
    BYTE                 passcodeEntry[XONLINE_PASSCODE_LENGTH];
    DWORD                cPasscodeEntered;
    BOOL                 fPasscodeError;
    // retrieved-game-invite start
    BOOL                 fInviteLogon;
    XONLINE_FRIEND       inviteFriend;

    // ---- friends ----
    XONLINETASK_HANDLE   hFriendsStartupTask;
    XONLINETASK_HANDLE   hFriendsEnumTask[XONLINE_MAX_LOGON_USERS];
    DWORD                dwFriendsUser;    // user index driving the friends screen
    DWORD                dwFriendsPort;    // controller that controls the screen
    BOOL                 fSignOutEnabled;
    XONLINE_FRIEND       friendsSnap[MAX_FRIENDS];
    DWORD                cFriendsSnap;
    DWORD                iFriendSel;
    BOOL                 fFriendMenuOpen;
    DWORD                iFriendMenuSel;
    XONLINE_FRIEND       exitFriend;       // exit data for JOIN_GAME exits
    WCHAR                wszStatus[64];    // one-line operation status
    // ILiveFriendsList objects (engine-owned, one per user index)
    struct FRIENDS_LIST_OBJ
    {
        RXDK_UIX_ENGINE *pEngine;
        DWORD            dwUserIndex;
        DWORD            dwConsumedStamp;
    }                    friendsLists[XONLINE_MAX_LOGON_USERS];

    // ---- players ----
    CPlayersList         playersList;
    UIX_PLAYERS_PARAMS   playersParams;
    DWORD                dwPlayersPort;    // UIX_INVALID_VALUE = lobby (all ports)
    DWORD                iPlayerSel;
    BOOL                 fPlayerMenuOpen;
    DWORD                iPlayerMenuSel;
    const ITitlePlayersListItem *view[RXDK_UIX_MAX_PLAYERS]; // filtered/sorted view
    DWORD                cView;

    // ---- notifications ----
    DWORD                dwNotifyFlags[XONLINE_MAX_LOGON_USERS];
    DWORD                dwNotifyPollCountdown;

    // ---- session published by the title (for invite-to-game) ----
    XNKID                sessionID;
    BOOL                 fHaveSession;

    // ---- exit / selection reporting ----
    BOOL                 fExitPending;
    UIX_EXIT_INFO        exitInfo;
    UIX_LOGON_EXIT_DATA  logonExitData;
    UIX_SELECTION_INFO   selInfo;

    // ---- title extension features (IUIXFeature) + skin ----
    UIX_SKIN            *pSkin;             // parsed .uix (NULL if none/failed)
    IPluginSupport      *pPluginSupport;    // skin-backed, handed to feature + plugin
    BOOL                 fPluginSupportSet; // pUIPlugin->SetPluginSupport() done
    CUIXEngineInternal   engineInternal;    // vtable handed to the active feature
    IUIXFeature         *pCustomFeature;    // active extension feature (UIXST_CUSTOM)
    UIX_FEATURE          customFeatureID;   // its token (== pCustomFeature) for exit info
    UIX_FEATURE_CONTEXT  customCtx;         // context passed to the feature
    BOOL                 fCustomEndRequested; // feature called EndFeature() this frame
    RXDK_UIX_SCREEN      screens[RXDK_UIX_MAX_SCREENS]; // registry
    DWORD                cScreens;
    UIX_SCREEN           screenStack[RXDK_UIX_MAX_SCREENS]; // shown, top = last
    DWORD                cScreenStack;
    DWORD                dwNextScreenInstance;
    DWORD                dwCustomPrevPressed[XONLINE_MAX_LOGON_USERS]; // rich edge-detect
};

static RXDK_UIX_ENGINE *Eng(LiveEngine *pThis)
{
    return (RXDK_UIX_ENGINE *)pThis;
}

// virtual keys decoded from XINPUT (edge-triggered)
#define UIXKEY_A     0x001
#define UIXKEY_B     0x002
#define UIXKEY_X     0x004
#define UIXKEY_Y     0x008
#define UIXKEY_UP    0x010
#define UIXKEY_DOWN  0x020
#define UIXKEY_LEFT  0x040
#define UIXKEY_RIGHT 0x080
#define UIXKEY_START 0x100
#define UIXKEY_LT    0x200
#define UIXKEY_RT    0x400

#define UIX_ANALOG_PRESS_THRESHOLD 0x20
#define UIX_THUMB_DEADZONE         20000

static DWORD DecodePressed(const XINPUT_STATE *pState)
{
    DWORD k = 0;
    const XINPUT_GAMEPAD *g = &pState->Gamepad;
    if (g->bAnalogButtons[XINPUT_GAMEPAD_A] > UIX_ANALOG_PRESS_THRESHOLD)
        k |= UIXKEY_A;
    if (g->bAnalogButtons[XINPUT_GAMEPAD_B] > UIX_ANALOG_PRESS_THRESHOLD)
        k |= UIXKEY_B;
    if (g->bAnalogButtons[XINPUT_GAMEPAD_X] > UIX_ANALOG_PRESS_THRESHOLD)
        k |= UIXKEY_X;
    if (g->bAnalogButtons[XINPUT_GAMEPAD_Y] > UIX_ANALOG_PRESS_THRESHOLD)
        k |= UIXKEY_Y;
    if (g->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > UIX_ANALOG_PRESS_THRESHOLD)
        k |= UIXKEY_LT;
    if (g->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > UIX_ANALOG_PRESS_THRESHOLD)
        k |= UIXKEY_RT;
    if ((g->wButtons & XINPUT_GAMEPAD_DPAD_UP) || g->sThumbLY > UIX_THUMB_DEADZONE)
        k |= UIXKEY_UP;
    if ((g->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) || g->sThumbLY < -UIX_THUMB_DEADZONE)
        k |= UIXKEY_DOWN;
    if (g->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
        k |= UIXKEY_LEFT;
    if (g->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
        k |= UIXKEY_RIGHT;
    if (g->wButtons & XINPUT_GAMEPAD_START)
        k |= UIXKEY_START;
    return k;
}

static void SetExit(RXDK_UIX_ENGINE *e, UIX_FEATURE feature, UIX_EXIT_CODE_TYPE code,
                    HRESULT hr, PVOID pData)
{
    e->exitInfo.FeatureID = feature;
    e->exitInfo.ExitCode  = code;
    e->exitInfo.hr        = hr;
    e->exitInfo.pExitData = pData;
    e->fExitPending       = TRUE;
    e->state              = UIXST_EXIT_PENDING;
}

// widen an ASCII gamertag for the font renderer
static void WidenTag(const CHAR *src, WCHAR *dst, size_t cch)
{
    size_t i = 0;
    for (; i + 1 < cch && src[i]; i++)
        dst[i] = (WCHAR)(unsigned char)src[i];
    dst[i] = 0;
}

static void AppendW(WCHAR *dst, size_t cch, const WCHAR *src)
{
    size_t n = 0;
    while (dst[n] && n < cch)
        n++;
    for (size_t i = 0; src[i] && n + 1 < cch; i++)
        dst[n++] = src[i];
    dst[n] = 0;
}

// ------------------------------------------------------------ logon logic ---

// picker rows: accounts first, then one "guest of" row per claimed real account
// that still has guest capacity (max 3 guests, and never beyond LogonUserCount).
static DWORD GuestsOf(RXDK_UIX_ENGINE *e, LONG iAccount)
{
    DWORD n = 0;
    for (DWORD p = 0; p < XONLINE_MAX_LOGON_USERS; p++)
        if (e->claims[p].iAccount == iAccount && e->claims[p].fGuest)
            n++;
    return n;
}

static BOOL AccountClaimed(RXDK_UIX_ENGINE *e, LONG iAccount, DWORD *pdwPort)
{
    for (DWORD p = 0; p < XONLINE_MAX_LOGON_USERS; p++) {
        if (e->claims[p].iAccount == iAccount && !e->claims[p].fGuest) {
            if (pdwPort)
                *pdwPort = p;
            return TRUE;
        }
    }
    return FALSE;
}

// enumerate the guest rows: returns account index for guest row n (or -1)
static LONG GuestRowAccount(RXDK_UIX_ENGINE *e, DWORD n)
{
    DWORD seen = 0;
    for (DWORD i = 0; i < e->cAccounts; i++) {
        if (AccountClaimed(e, (LONG)i, NULL) && GuestsOf(e, (LONG)i) < 3) {
            if (seen == n)
                return (LONG)i;
            seen++;
        }
    }
    return -1;
}

static DWORD GuestRowCount(RXDK_UIX_ENGINE *e)
{
    DWORD n = 0;
    while (GuestRowAccount(e, n) >= 0)
        n++;
    return n;
}

static DWORD PickerRowCount(RXDK_UIX_ENGINE *e)
{
    return e->cAccounts + GuestRowCount(e);
}

static void StartLogonTask(RXDK_UIX_ENGINE *e)
{
    memset(e->logonUsers, 0, sizeof(e->logonUsers));

    // fill slots in claim order
    for (DWORD k = 0; k < e->cClaims; k++) {
        DWORD port = e->claimOrder[k];
        const UIX_CLAIM *c = &e->claims[port];
        e->logonUsers[k] = e->accounts[c->iAccount];
        if (c->fGuest)
            XOnlineSetUserGuestNumber(e->logonUsers[k].xuid.dwUserFlags, c->dwGuestNo);
        e->logonExitData.pMappedControllers[k] = port;
    }

    DWORD cServices = 0;
    while (cServices < UIX_MAX_LOGON_SERVICES && e->logonParams.LogonServiceIDs[cServices])
        cServices++;

    HRESULT hr = XOnlineLogon(e->logonUsers, cServices ? e->logonParams.LogonServiceIDs : NULL,
                              cServices, NULL, &e->hLogonTask);
    if (FAILED(hr)) {
        e->hLogonTask = NULL;
        SetExit(e, _uix_logon_feature, UIX_EXIT_LOGON_FAILED, hr, NULL);
        return;
    }
    e->state = UIXST_LOGON_CONNECTING;
}

// claim the currently highlighted picker row for `port`; may divert to passcode
static void ClaimRow(RXDK_UIX_ENGINE *e, DWORD port)
{
    if (e->claims[port].iAccount >= 0)
        return; // this controller already claimed
    if (e->cClaims >= e->logonParams.LogonUserCount ||
        e->cClaims >= XONLINE_MAX_LOGON_USERS)
        return; // party is full

    if (e->iSelected < e->cAccounts) {
        LONG iAccount = (LONG)e->iSelected;
        if (AccountClaimed(e, iAccount, NULL))
            return; // an account can be claimed once (guests use the guest rows)

        if (e->accounts[iAccount].dwUserOptions & XONLINE_USER_OPTION_REQUIRE_PASSCODE) {
            e->dwPasscodePort    = port;
            e->iPasscodeAccount  = iAccount;
            e->cPasscodeEntered  = 0;
            e->fPasscodeError    = FALSE;
            e->state             = UIXST_LOGON_PASSCODE;
            return;
        }
        e->claims[port].iAccount = iAccount;
        e->claims[port].fGuest   = FALSE;
        e->claimOrder[e->cClaims++] = port;
    } else {
        LONG iAccount = GuestRowAccount(e, e->iSelected - e->cAccounts);
        if (iAccount < 0)
            return;
        e->claims[port].iAccount  = iAccount;
        e->claims[port].fGuest    = TRUE;
        e->claims[port].dwGuestNo = GuestsOf(e, iAccount); // 1..3 (this claim now counted)
        e->claimOrder[e->cClaims++] = port;
    }

    // single-user logon starts on the first claim (the common title flow)
    if (e->logonParams.LogonUserCount <= 1)
        StartLogonTask(e);
}

static void UnclaimPort(RXDK_UIX_ENGINE *e, DWORD port)
{
    if (e->claims[port].iAccount < 0)
        return;
    // un-claiming a real account also drops its guests
    if (!e->claims[port].fGuest) {
        LONG iAccount = e->claims[port].iAccount;
        for (DWORD p = 0; p < XONLINE_MAX_LOGON_USERS; p++)
            if (e->claims[p].fGuest && e->claims[p].iAccount == iAccount)
                e->claims[p].iAccount = -1;
    }
    e->claims[port].iAccount = -1;
    e->claims[port].fGuest   = FALSE;
    // rebuild claim order
    DWORD k = 0;
    for (DWORD i = 0; i < e->cClaims; i++) {
        DWORD p = e->claimOrder[i];
        if (e->claims[p].iAccount >= 0)
            e->claimOrder[k++] = p;
    }
    e->cClaims = k;
}

// map a passcode button edge to the XONLINE_PASSCODE_TYPE code
static BYTE PasscodeCode(DWORD edges)
{
    if (edges & UIXKEY_UP)    return XONLINE_PASSCODE_DPAD_UP;
    if (edges & UIXKEY_DOWN)  return XONLINE_PASSCODE_DPAD_DOWN;
    if (edges & UIXKEY_LEFT)  return XONLINE_PASSCODE_DPAD_LEFT;
    if (edges & UIXKEY_RIGHT) return XONLINE_PASSCODE_DPAD_RIGHT;
    if (edges & UIXKEY_X)     return XONLINE_PASSCODE_GAMEPAD_X;
    if (edges & UIXKEY_Y)     return XONLINE_PASSCODE_GAMEPAD_Y;
    if (edges & UIXKEY_LT)    return XONLINE_PASSCODE_GAMEPAD_LEFT_TRIGGER;
    if (edges & UIXKEY_RT)    return XONLINE_PASSCODE_GAMEPAD_RIGHT_TRIGGER;
    return 0;
}

// --------------------------------------------------------- friends helpers ---

static void EnsureFriendsMachinery(RXDK_UIX_ENGINE *e, DWORD dwUserIndex)
{
    if (!e->hFriendsStartupTask)
        XOnlineFriendsStartup(NULL, &e->hFriendsStartupTask);
    if (dwUserIndex < XONLINE_MAX_LOGON_USERS && !e->hFriendsEnumTask[dwUserIndex])
        XOnlineFriendsEnumerate(dwUserIndex, NULL, &e->hFriendsEnumTask[dwUserIndex]);
}

static void PumpTask(XONLINETASK_HANDLE *phTask)
{
    if (!*phTask)
        return;
    HRESULT hr = XOnlineTaskContinue(*phTask);
    if (hr != XONLINETASK_S_RUNNING && hr != XONLINETASK_S_RUNNING_IDLE && FAILED(hr)) {
        XOnlineTaskClose(*phTask);
        *phTask = NULL;
    }
}

static DWORD FriendsSnapshotStamp(const XONLINE_FRIEND *pFriends, DWORD cFriends)
{
    DWORD stamp = cFriends;
    for (DWORD i = 0; i < cFriends; i++)
        stamp ^= (DWORD)pFriends[i].xuid.qwUserID + pFriends[i].dwFriendState + i;
    return stamp;
}

// friend action menu construction
#define FRMENU_MAX 8
enum FRIEND_ACTION
{
    FRACT_NONE = 0,
    FRACT_ACCEPT_REQUEST,
    FRACT_DECLINE_REQUEST,
    FRACT_BLOCK_REQUEST,
    FRACT_ACCEPT_INVITE,
    FRACT_DECLINE_INVITE,
    FRACT_JOIN,
    FRACT_INVITE,
    FRACT_REMOVE,
    FRACT_CANCEL,
};

static DWORD BuildFriendMenu(RXDK_UIX_ENGINE *e, const XONLINE_FRIEND *f,
                             FRIEND_ACTION *pActions, const WCHAR **pLabels)
{
    DWORD n = 0;
    DWORD s = f->dwFriendState;
    if (s & XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST) {
        pActions[n] = FRACT_ACCEPT_REQUEST;  pLabels[n++] = L"Accept friend request";
        pActions[n] = FRACT_DECLINE_REQUEST; pLabels[n++] = L"Decline friend request";
        pActions[n] = FRACT_BLOCK_REQUEST;   pLabels[n++] = L"Block requests";
    }
    if (s & XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE) {
        pActions[n] = FRACT_ACCEPT_INVITE;   pLabels[n++] = L"Accept game invite";
        pActions[n] = FRACT_DECLINE_INVITE;  pLabels[n++] = L"Decline game invite";
    } else if ((s & XONLINE_FRIENDSTATE_FLAG_JOINABLE) &&
               XOnlineTitleIdIsSameTitle(f->dwTitleID)) {
        pActions[n] = FRACT_JOIN;            pLabels[n++] = L"Join game";
    }
    if (e->fHaveSession &&
        e->dwProperties[UIX_PROPERTY_ALLOW_GAME_INVITES] &&
        (s & XONLINE_FRIENDSTATE_FLAG_ONLINE)) {
        pActions[n] = FRACT_INVITE;          pLabels[n++] = L"Invite to game";
    }
    pActions[n] = FRACT_REMOVE;              pLabels[n++] = L"Remove friend";
    pActions[n] = FRACT_CANCEL;              pLabels[n++] = L"Cancel";
    return n;
}

static void FriendOpStatus(RXDK_UIX_ENGINE *e, HRESULT hr, const WCHAR *ok, const WCHAR *fail)
{
    e->wszStatus[0] = 0;
    AppendW(e->wszStatus, 64, SUCCEEDED(hr) ? ok : fail);
}

// --------------------------------------------------------- players helpers ---

static void BuildPlayersView(RXDK_UIX_ENGINE *e)
{
    const ITitlePlayersListItem **src;
    DWORD cSrc;
    if (e->playersParams.DisplayType & UIX_PLAYERS_DISPLAY_DEPARTED_PLAYERS) {
        src  = e->playersList.m_Departed;
        cSrc = e->playersList.m_cDeparted;
    } else {
        src  = e->playersList.m_Current;
        cSrc = e->playersList.m_cCurrent;
    }

    DWORD filter = e->playersParams.FilterFlags ? e->playersParams.FilterFlags
                                                : e->playersList.m_FilterFlags;
    e->cView = 0;
    // the controlling player is shown first (unless the list is sorted)
    const ITitlePlayersListItem *pFirst =
        e->playersParams.SortCurrentPlayersList ? NULL : e->playersParams.pPlayerControllingScreen;
    if (pFirst)
        e->view[e->cView++] = pFirst;
    for (DWORD i = 0; i < cSrc && e->cView < RXDK_UIX_MAX_PLAYERS; i++) {
        const ITitlePlayersListItem *p = src[i];
        if (p == pFirst)
            continue;
        if (filter && !(((ITitlePlayersListItem *)p)->GetBitflags() & filter))
            continue;
        e->view[e->cView++] = p;
    }

    if (e->playersParams.SortCurrentPlayersList) {
        // insertion sort via the item's own Compare
        for (DWORD i = 1; i < e->cView; i++) {
            const ITitlePlayersListItem *key = e->view[i];
            DWORD j = i;
            while (j > 0 &&
                   ((ITitlePlayersListItem *)e->view[j - 1])->Compare(key) > 0) {
                e->view[j] = e->view[j - 1];
                j--;
            }
            e->view[j] = key;
        }
    }
}

// =========================================================================
//                              C API surface
// =========================================================================

extern "C" {

// Read an entire file into a heap buffer (used for the .uix skin). Returns NULL on
// any failure; *pcb gets the byte count on success.
static BYTE *UixReadFile(LPCSTR pPath, DWORD *pcb)
{
    *pcb = 0;
    HANDLE h = CreateFileA(pPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return NULL;
    DWORD cb = GetFileSize(h, NULL);
    BYTE *p = NULL;
    if (cb != 0 && cb != INVALID_FILE_SIZE) {
        p = (BYTE *)UixAlloc(cb);
        DWORD got = 0;
        if (!p || !ReadFile(h, p, cb, &got, NULL) || got != cb) {
            if (p) UixFree(p);
            p = NULL;
        }
    }
    CloseHandle(h);
    if (p) *pcb = cb;
    return p;
}

XBOXAPI HRESULT WINAPI UIXCreateLiveEngine(LPCSTR pSkinFileName, DWORD LanguageID,
                                           ILiveEngine **ppEngine)
{
    // The built-in features render as plain text (font-renderer contract -- see the
    // file header) and ignore the skin. But TITLE extension features (IUIXFeature,
    // e.g. the on-screen keyboard) pull geometry/labels/textures from the .uix skin
    // through IPluginSupport, so load it when present.
    if (!ppEngine)
        return E_POINTER;
    RXDK_UIX_ENGINE *e = (RXDK_UIX_ENGINE *)UixAlloc(sizeof(RXDK_UIX_ENGINE));
    if (!e) {
        *ppEngine = NULL;
        return E_OUTOFMEMORY;
    }
    memset(e, 0, sizeof(*e));
    new (&e->playersList) CPlayersList;
    e->playersList.m_cCurrent    = 0;
    e->playersList.m_cDeparted   = 0;
    e->playersList.m_FilterFlags = 0;
    new (&e->engineInternal) CUIXEngineInternal;
    e->engineInternal.pEngine = e;
    e->dwNextScreenInstance = 1;

    // Load the skin (best-effort: a missing/unparseable skin leaves built-in
    // features working; only extension features that need skin resources fail).
    if (pSkinFileName && pSkinFileName[0]) {
        DWORD cb = 0;
        BYTE *pBytes = UixReadFile(pSkinFileName, &cb);
        if (pBytes) {
            e->pSkin = UixSkinLoad(pBytes, cb, LanguageID);
            UixFree(pBytes);
            if (e->pSkin)
                e->pPluginSupport = UixCreatePluginSupport(e->pSkin);
        }
    }

    e->lRefCount = 1;
    e->state     = UIXST_IDLE;
    for (DWORD p = 0; p < XONLINE_MAX_LOGON_USERS; p++) {
        e->claims[p].iAccount = -1;
        e->friendsLists[p].pEngine     = e;
        e->friendsLists[p].dwUserIndex = p;
    }
    // retail defaults: notifications shown, game invites allowed
    e->dwProperties[UIX_PROPERTY_DISPLAY_NOTIFICATIONS] = TRUE;
    e->dwProperties[UIX_PROPERTY_ALLOW_GAME_INVITES]    = TRUE;
    *ppEngine = (ILiveEngine *)e;
    return S_OK;
}

XBOXAPI HRESULT WINAPI UIXCreateUIPlugin(ITitleFontRenderer *pFont, ITitleUIPlugin **ppUIPlugin)
{
    if (!ppUIPlugin)
        return E_POINTER;
    CDefaultUIPlugin *p = (CDefaultUIPlugin *)UixAlloc(sizeof(CDefaultUIPlugin));
    if (!p) {
        *ppUIPlugin = NULL;
        return E_OUTOFMEMORY;
    }
    p = new (p) CDefaultUIPlugin;
    p->m_pFont = pFont;
    p->m_cRef  = 1;
    *ppUIPlugin = p;
    return S_OK;
}

XBOXAPI HRESULT WINAPI UIXCreateAudioPlugin(VOID *pXactEngine, VOID *pXactSoundBank,
                                            ITitleAudioPlugin **ppAudioPlugin)
{
    // A functional no-op object: skin sound banks never load by design.
    (void)pXactEngine;
    (void)pXactSoundBank;
    if (!ppAudioPlugin)
        return E_POINTER;
    CDefaultAudioPlugin *p = (CDefaultAudioPlugin *)UixAlloc(sizeof(CDefaultAudioPlugin));
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
        for (DWORD i = 0; i < XONLINE_MAX_LOGON_USERS; i++)
            if (e->hFriendsEnumTask[i])
                XOnlineFriendsEnumerateFinish(e->hFriendsEnumTask[i]);
        if (e->hFriendsStartupTask)
            XOnlineTaskClose(e->hFriendsStartupTask);
        if (e->pPluginSupport)
            UixDestroyPluginSupport(e->pPluginSupport);
        if (e->pSkin)
            UixSkinFree(e->pSkin);
        UixFree(e);
        return 0;
    }
    return (ULONG)cRef;
}

HRESULT WINAPI LiveEngine_SetUIPlugin(LiveEngine *pThis, ITitleUIPlugin *pUIPlugin)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    e->pUIPlugin = pUIPlugin;
    // Hand the title's plugin the skin-backed IPluginSupport it needs to resolve
    // layouts/images/strings (extension-feature rendering path).
    if (pUIPlugin && e->pPluginSupport && !e->fPluginSupportSet) {
        pUIPlugin->SetPluginSupport(e->pPluginSupport);
        e->fPluginSupportSet = TRUE;
    }
    return S_OK;
}

HRESULT WINAPI LiveEngine_SetAudioPlugin(LiveEngine *pThis, ITitleAudioPlugin *pAudioPlugin)
{
    Eng(pThis)->pAudioPlugin = pAudioPlugin;
    return S_OK;
}

// A UIX_FEATURE is a built-in when it points inside s_uixTokens; anything else is a
// title-supplied extension feature, and the token IS an IUIXFeature*.
static BOOL IsBuiltinFeature(UIX_FEATURE FeatureID)
{
    const BYTE *p = (const BYTE *)FeatureID;
    return (p >= s_uixTokens && p < s_uixTokens + 5);
}

// Populate the context handed to an extension feature (all five members must stay
// live for the feature's whole lifetime; customCtx is an engine member, so it does).
static void BuildCustomContext(RXDK_UIX_ENGINE *e)
{
    ITitleFontRenderer *pFont = NULL;
    if (e->pUIPlugin)
        e->pUIPlugin->GetFont(&pFont);
    e->customCtx.pEngine         = (ILiveEngine *)e;
    e->customCtx.pEngineInternal = &e->engineInternal;
    e->customCtx.pPluginSupport  = e->pPluginSupport;
    e->customCtx.pUIPlugin       = e->pUIPlugin;
    e->customCtx.pFont           = pFont;
}

HRESULT WINAPI LiveEngine_EnableFeature(LiveEngine *pThis, UIX_FEATURE FeatureID)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    const BYTE *p = (const BYTE *)FeatureID;
    if (p >= s_uixTokens && p < s_uixTokens + 4) {
        e->dwEnabledFeatures |= (1u << (p - s_uixTokens));
        return S_OK;
    }
    if (IsBuiltinFeature(FeatureID))
        return E_INVALIDARG;  // s_uixTokens[4] is the voice-mail entry point, not a feature

    // Extension feature: give it its context and let it register.
    IUIXFeature *pFeature = (IUIXFeature *)FeatureID;
    BuildCustomContext(e);
    HRESULT hr = pFeature->SetContext(&e->customCtx);
    if (SUCCEEDED(hr))
        hr = pFeature->CreateFeature();
    return hr;
}

HRESULT WINAPI LiveEngine_StartFeature(LiveEngine *pThis, UIX_FEATURE FeatureID,
                                       const VOID *pFeatureParams)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);

    if (e->state != UIXST_IDLE)
        return E_FAIL; // a feature screen is already active (or an exit is unread)

    // ----- LOGON -----
    if (FeatureID == _uix_logon_feature) {
        if (!(e->dwEnabledFeatures & 0x1))
            return E_FAIL;

        const UIX_LOGON_PARAMS *pParams = (const UIX_LOGON_PARAMS *)pFeatureParams;
        if (!pParams || pParams->StructSize != sizeof(UIX_LOGON_PARAMS))
            return E_INVALIDARG;

        if (e->hLogonTask) { // leaving a previous session
            XOnlineTaskClose(e->hLogonTask);
            e->hLogonTask = NULL;
            e->fLoggedOn  = FALSE;
        }

        e->logonParams = *pParams;
        if (e->logonParams.LogonUserCount == 0)
            e->logonParams.LogonUserCount = 1;
        if (e->logonParams.LogonUserCount > XONLINE_MAX_LOGON_USERS)
            e->logonParams.LogonUserCount = XONLINE_MAX_LOGON_USERS;
        memset(&e->logonExitData, 0xFF, sizeof(e->logonExitData));
        e->fExitPending = FALSE;
        e->fInviteLogon = FALSE;
        e->cClaims      = 0;
        for (DWORD p = 0; p < XONLINE_MAX_LOGON_USERS; p++)
            e->claims[p].iAccount = -1;
        memset(e->dwPrevPressed, 0, sizeof(e->dwPrevPressed));

        // RETRIEVED_STATE: sign the saved users straight in
        if (pParams->LogonType == UIX_LOGON_TYPE_RETRIEVED_STATE) {
            if (!pParams->pLogonState)
                return E_INVALIDARG;
            DWORD cServices = UIX_MAX_LOGON_SERVICES;
            HRESULT hr = XOnlineRetrieveLogonState(pParams->pLogonState, e->logonUsers,
                                                   e->logonParams.LogonServiceIDs, &cServices);
            if (FAILED(hr))
                return hr;
            hr = XOnlineLogon(e->logonUsers, cServices ? e->logonParams.LogonServiceIDs : NULL,
                              cServices, NULL, &e->hLogonTask);
            if (FAILED(hr)) {
                e->hLogonTask = NULL;
                SetExit(e, _uix_logon_feature, UIX_EXIT_LOGON_FAILED, hr, NULL);
                return S_OK;
            }
            e->logonExitData.pMappedControllers[0] = 0;
            e->state = UIXST_LOGON_CONNECTING;
            return S_OK;
        }

        // enumerate stored accounts (hard disk + MUs)
        e->cAccounts = 0;
        HRESULT hr = XOnlineGetUsers(e->accounts, &e->cAccounts);
        if (FAILED(hr)) {
            SetExit(e, _uix_logon_feature, UIX_EXIT_LOGON_FAILED, hr, NULL);
            return S_OK;
        }
        if (e->cAccounts == 0) {
            SetExit(e, _uix_logon_feature, UIX_EXIT_LOGON_FAILED, XONLINE_E_USER_NOT_PRESENT, NULL);
            return S_OK;
        }

        // RETRIEVED_GAME_INVITE: sign in the users recorded in the invite
        if (pParams->LogonType == UIX_LOGON_TYPE_RETRIEVED_GAME_INVITE) {
            if (!pParams->pGameInvite)
                return E_INVALIDARG;
            const XONLINE_ACCEPTED_GAMEINVITE *pInvite = pParams->pGameInvite;
            memset(e->logonUsers, 0, sizeof(e->logonUsers));
            DWORD cUsers = 0;
            for (DWORD i = 0; i < XONLINE_MAX_LOGON_USERS; i++) {
                if (!pInvite->xuidLogonUsers[i].qwUserID)
                    continue;
                for (DWORD a = 0; a < e->cAccounts; a++) {
                    if (e->accounts[a].xuid.qwUserID == pInvite->xuidLogonUsers[i].qwUserID) {
                        e->logonUsers[cUsers] = e->accounts[a];
                        e->logonExitData.pMappedControllers[cUsers] = i;
                        cUsers++;
                        break;
                    }
                }
            }
            if (cUsers == 0) {
                SetExit(e, _uix_logon_feature, UIX_EXIT_LOGON_FAILED,
                        XONLINE_E_USER_NOT_PRESENT, NULL);
                return S_OK;
            }
            DWORD cServices = 0;
            while (cServices < UIX_MAX_LOGON_SERVICES && e->logonParams.LogonServiceIDs[cServices])
                cServices++;
            hr = XOnlineLogon(e->logonUsers, cServices ? e->logonParams.LogonServiceIDs : NULL,
                              cServices, NULL, &e->hLogonTask);
            if (FAILED(hr)) {
                e->hLogonTask = NULL;
                SetExit(e, _uix_logon_feature, UIX_EXIT_LOGON_FAILED, hr, NULL);
                return S_OK;
            }
            e->fInviteLogon = TRUE;
            e->inviteFriend = pInvite->InvitingFriend;
            e->state = UIXST_LOGON_CONNECTING;
            return S_OK;
        }

        e->iSelected = 0;
        if (pParams->LogonType == UIX_LOGON_TYPE_SILENT) {
            // no UI: first stored account in slot 0 (see XOnlineSilentLogon)
            e->claims[0].iAccount = 0;
            e->claims[0].fGuest   = FALSE;
            e->claimOrder[0]      = 0;
            e->cClaims            = 1;
            e->logonExitData.pMappedControllers[0] = 0;
            StartLogonTask(e);
        } else {
            e->state = UIXST_LOGON_PICKING;
        }
        return S_OK;
    }

    // ----- FRIENDS -----
    if (FeatureID == _uix_friends_feature) {
        if (!(e->dwEnabledFeatures & 0x2))
            return E_FAIL;
        // works for engine-driven AND title-driven (raw XOnlineLogon) sessions
        if (!XOnlineGetLogonUsers())
            return E_FAIL; // needs an established Live session

        const UIX_FRIENDS_PARAMS *pParams = (const UIX_FRIENDS_PARAMS *)pFeatureParams;
        if (!pParams || pParams->StructSize != sizeof(UIX_FRIENDS_PARAMS))
            return E_INVALIDARG;
        if (pParams->UserPort >= XONLINE_MAX_LOGON_USERS)
            return E_INVALIDARG;

        e->dwFriendsUser   = pParams->UserPort;
        e->dwFriendsPort   = pParams->UserPort;
        e->fSignOutEnabled = pParams->SignOutEnabled;
        e->fFriendMenuOpen = FALSE;
        e->wszStatus[0]    = 0;
        e->fExitPending    = FALSE;
        memset(e->dwPrevPressed, 0, sizeof(e->dwPrevPressed));

        EnsureFriendsMachinery(e, e->dwFriendsUser);

        // initial selection
        e->cFriendsSnap = XOnlineFriendsGetLatest(e->dwFriendsUser, MAX_FRIENDS, e->friendsSnap);
        e->iFriendSel = 0;
        if (pParams->SelectedFriendXUID.qwUserID) {
            for (DWORD i = 0; i < e->cFriendsSnap; i++) {
                if (e->friendsSnap[i].xuid.qwUserID == pParams->SelectedFriendXUID.qwUserID) {
                    e->iFriendSel = i;
                    break;
                }
            }
        }
        e->state = UIXST_FRIENDS;
        return S_OK;
    }

    // ----- PLAYERS -----
    if (FeatureID == _uix_players_feature) {
        if (!(e->dwEnabledFeatures & 0x4))
            return E_FAIL;

        const UIX_PLAYERS_PARAMS *pParams = (const UIX_PLAYERS_PARAMS *)pFeatureParams;
        if (!pParams || pParams->StructSize != sizeof(UIX_PLAYERS_PARAMS))
            return E_INVALIDARG;
        if (!(pParams->DisplayType &
              (UIX_PLAYERS_DISPLAY_CURRENT_PLAYERS | UIX_PLAYERS_DISPLAY_DEPARTED_PLAYERS)))
            return E_INVALIDARG;

        e->playersParams = *pParams;
        e->dwPlayersPort = pParams->UserPort;
        if (pParams->DisplayType & UIX_PLAYERS_DISPLAY_LOBBY_MODE)
            e->dwPlayersPort = UIX_INVALID_VALUE;
        e->iPlayerSel      = 0;
        e->fPlayerMenuOpen = FALSE;
        e->wszStatus[0]    = 0;
        e->fExitPending    = FALSE;
        memset(e->dwPrevPressed, 0, sizeof(e->dwPrevPressed));

        BuildPlayersView(e);
        // initial selection by XUID if requested
        if (pParams->pSelectedPlayerXUID) {
            for (DWORD i = 0; i < e->cView; i++) {
                const XUID *px = ((ITitlePlayersListItem *)e->view[i])->GetXUID();
                if (px && px->qwUserID == pParams->pSelectedPlayerXUID->qwUserID) {
                    e->iPlayerSel = i;
                    break;
                }
            }
        }
        e->state = UIXST_PLAYERS;
        return S_OK;
    }

    // ----- title extension feature (IUIXFeature) -----
    if (!IsBuiltinFeature(FeatureID)) {
        IUIXFeature *pFeature = (IUIXFeature *)FeatureID;
        BuildCustomContext(e);   // refresh (plugin may have been set after EnableFeature)
        e->cScreens         = 0;
        e->cScreenStack     = 0;
        e->fCustomEndRequested = FALSE;

        HRESULT hr = pFeature->ActivateFeature(pFeatureParams);
        if (FAILED(hr))
            return hr;

        // ActivateFeature constructed the feature's screens (each registered via
        // CreateScreen). Show the current one; its objects are created on show.
        UIX_SCREEN cur = NULL;
        if (SUCCEEDED(pFeature->GetCurrentScreen(&cur)) && cur) {
            e->engineInternal.EnableScreenInput(cur, TRUE);
            e->engineInternal.ShowScreen(cur, TRUE);
        }
        e->pCustomFeature  = pFeature;
        e->customFeatureID = FeatureID;
        memset(e->dwCustomPrevPressed, 0, sizeof(e->dwCustomPrevPressed));
        e->state = UIXST_CUSTOM;
        return S_OK;
    }

    // _uix_uitest_feature exists only in checked builds of retail uix
    return E_FAIL;
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
        e->fInputValid[Port]   = FALSE;
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

// consume edge-triggered virtual keys for one port
static DWORD PortEdges(RXDK_UIX_ENGINE *e, DWORD port)
{
    if (!e->fInputValid[port])
        return 0;
    DWORD pressed = DecodePressed(&e->input[port]);
    DWORD edges   = pressed & ~e->dwPrevPressed[port];
    e->dwPrevPressed[port] = pressed;
    return edges;
}

// ============================================================================
//  Extension-feature host: IUIXEngineInternal + a screen stack, driving a
//  title-supplied IUIXFeature (e.g. the UIXKeyboard sample's on-screen keyboard)
//  through its IUIXScreen(s). Rendering/object work is delegated to the title's
//  ITitleUIPlugin; skin resources come from the .uix via IPluginSupport.
// ============================================================================

static RXDK_UIX_SCREEN *ScreenRec(RXDK_UIX_ENGINE *e, UIX_SCREEN h)
{
    for (DWORD i = 0; i < e->cScreens; i++)
        if ((UIX_SCREEN)&e->screens[i] == h && e->screens[i].pScreen)
            return &e->screens[i];
    return NULL;
}

static RXDK_UIX_SCREEN *TopScreen(RXDK_UIX_ENGINE *e)
{
    if (e->cScreenStack == 0)
        return NULL;
    return ScreenRec(e, e->screenStack[e->cScreenStack - 1]);
}

static void EnsureScreenCreated(RXDK_UIX_ENGINE *e, RXDK_UIX_SCREEN *s)
{
    (void)e;
    if (s && !s->fCreated && s->pScreen) {
        s->fCreated = TRUE;
        s->pScreen->CreateScreen();   // screen-side: creates its objects via the engine
    }
}

HRESULT CUIXEngineInternal::CreateScreen(UIX_SCREEN *pScreenObject, DWORD ScreenResID,
                                         IUIXScreen *pScreenInterface)
{
    if (!pScreenObject)
        return E_POINTER;
    *pScreenObject = NULL;
    if (pEngine->cScreens >= RXDK_UIX_MAX_SCREENS)
        return E_OUTOFMEMORY;
    RXDK_UIX_SCREEN *s = &pEngine->screens[pEngine->cScreens++];
    s->pScreen         = pScreenInterface;
    s->dwScreenResID   = ScreenResID;
    s->dwInstance      = pEngine->dwNextScreenInstance++;
    s->fCreated        = FALSE;
    s->fInputEnabled   = FALSE;
    s->fAllowStartBack = FALSE;
    *pScreenObject = (UIX_SCREEN)s;
    return S_OK;
}

HRESULT CUIXEngineInternal::DestroyScreen(UIX_SCREEN ScreenObject)
{
    RXDK_UIX_SCREEN *s = ScreenRec(pEngine, ScreenObject);
    if (!s)
        return E_INVALIDARG;
    if (pEngine->pUIPlugin)
        pEngine->pUIPlugin->DestroyScreenObjects(s->dwInstance);
    for (DWORD i = 0; i < pEngine->cScreenStack; ) {
        if (pEngine->screenStack[i] == ScreenObject) {
            for (DWORD j = i + 1; j < pEngine->cScreenStack; j++)
                pEngine->screenStack[j - 1] = pEngine->screenStack[j];
            pEngine->cScreenStack--;
        } else i++;
    }
    s->pScreen = NULL;   // free the record
    return S_OK;
}

HRESULT CUIXEngineInternal::ShowScreen(UIX_SCREEN ScreenObject, BOOL ReplaceCurrent)
{
    RXDK_UIX_SCREEN *s = ScreenRec(pEngine, ScreenObject);
    if (!s)
        return E_INVALIDARG;
    if (ReplaceCurrent && pEngine->cScreenStack > 0)
        pEngine->cScreenStack--;
    for (DWORD i = 0; i < pEngine->cScreenStack; i++)
        if (pEngine->screenStack[i] == ScreenObject)
            return S_OK;   // already shown
    if (pEngine->cScreenStack >= RXDK_UIX_MAX_SCREENS)
        return E_OUTOFMEMORY;
    EnsureScreenCreated(pEngine, s);
    s->pScreen->ReceiveMessage(UIX_SCREENMSG_SHOW, NULL);
    pEngine->screenStack[pEngine->cScreenStack++] = ScreenObject;
    return S_OK;
}

HRESULT CUIXEngineInternal::HideTopScreen()
{
    if (pEngine->cScreenStack == 0)
        return S_OK;
    RXDK_UIX_SCREEN *s = TopScreen(pEngine);
    if (s && s->pScreen)
        s->pScreen->ReceiveMessage(UIX_SCREENMSG_HIDE, NULL);
    pEngine->cScreenStack--;
    return S_OK;
}

HRESULT CUIXEngineInternal::EnableScreenInput(UIX_SCREEN ScreenObject, BOOL Enable)
{
    RXDK_UIX_SCREEN *s = ScreenRec(pEngine, ScreenObject);
    if (!s) return E_INVALIDARG;
    s->fInputEnabled = Enable;
    return S_OK;
}

HRESULT CUIXEngineInternal::AllowStartAndBack(UIX_SCREEN ScreenObject, BOOL Allow)
{
    RXDK_UIX_SCREEN *s = ScreenRec(pEngine, ScreenObject);
    if (!s) return E_INVALIDARG;
    s->fAllowStartBack = Allow;
    return S_OK;
}

HRESULT CUIXEngineInternal::CreateObject(DWORD *pObjectID, UIX_SCREEN ScreenObject,
                                         UIX_OBJECT_TYPE ObjectType, DWORD ObjectResID)
{
    if (!pObjectID) return E_POINTER;
    *pObjectID = 0;
    RXDK_UIX_SCREEN *s = ScreenRec(pEngine, ScreenObject);
    if (!s || !pEngine->pUIPlugin) return E_FAIL;
    // Engine's CreateObject carries no ScreenResID; the plugin needs it plus our
    // per-screen instance id, both remembered at CreateScreen time.
    return pEngine->pUIPlugin->CreateObject(ObjectType, s->dwInstance, s->dwScreenResID,
                                            ObjectResID, pObjectID);
}

HRESULT CUIXEngineInternal::SetText(UIX_SCREEN, DWORD ObjectID, LPCWSTR pText)
{
    if (!pEngine->pUIPlugin) return E_FAIL;
    return pEngine->pUIPlugin->SetText(ObjectID, 0, pText ? pText : L"", 0, NULL);
}

HRESULT CUIXEngineInternal::SetTextWithResID(UIX_SCREEN, DWORD ObjectID, DWORD StringResID)
{
    if (!pEngine->pUIPlugin) return E_FAIL;
    const WCHAR *pText = pEngine->pSkin ? UixSkinGetString(pEngine->pSkin, StringResID) : NULL;
    return pEngine->pUIPlugin->SetText(ObjectID, 0, pText ? pText : L"", 0, NULL);
}

HRESULT CUIXEngineInternal::AddListItem(UIX_SCREEN, DWORD ObjectID, LPCWSTR pText,
                                        DWORD IconCount, const UIX_SKIN_ICON_INFO *pIconInfo)
{
    if (!pEngine->pUIPlugin) return E_FAIL;
    DWORD idx = 0;
    pEngine->pUIPlugin->InsertItem(ObjectID, 0xFFFFFFFF, &idx);   // append
    return pEngine->pUIPlugin->SetText(ObjectID, idx, pText ? pText : L"", IconCount, pIconInfo);
}

HRESULT CUIXEngineInternal::AddGreyListItem(UIX_SCREEN, DWORD ObjectID, LPCWSTR pText,
                                            DWORD IconCount, const UIX_SKIN_ICON_INFO *pIconInfo)
{
    if (!pEngine->pUIPlugin) return E_FAIL;
    DWORD idx = 0;
    pEngine->pUIPlugin->InsertItem(ObjectID, 0xFFFFFFFF, &idx);
    pEngine->pUIPlugin->SetText(ObjectID, idx, pText ? pText : L"", IconCount, pIconInfo);
    pEngine->pUIPlugin->SetObjectState(ObjectID, idx, UIX_OBJSTATE_LIST_ITEM_GREYED, TRUE);
    return S_OK;
}

HRESULT CUIXEngineInternal::ClearList(UIX_SCREEN, DWORD ObjectID, BOOL ResetSelectionIndex)
{
    if (!pEngine->pUIPlugin) return E_FAIL;
    return pEngine->pUIPlugin->Clear(ObjectID, ResetSelectionIndex);
}

HRESULT CUIXEngineInternal::SeparateTextAndIcons(LPCWSTR *ppText, DWORD *pIconCount,
                                                 UIX_SKIN_ICON_INFO **ppIcons)
{
    // The skin loader already strips the icon block from strings, so text passed
    // through here has no embedded {IMG_*} icons to separate.
    (void)ppText;
    if (pIconCount) *pIconCount = 0;
    if (ppIcons)    *ppIcons    = NULL;
    return S_OK;
}

HRESULT CUIXEngineInternal::SendMessageToAllFeatures(UIX_FEATUREMSG_TYPE Msg, const VOID *pParam)
{
    if (pEngine->pCustomFeature)
        pEngine->pCustomFeature->ReceiveMessage(Msg, pParam, NULL, NULL);
    return S_OK;
}

HRESULT CUIXEngineInternal::PlaySound(DWORD SoundResID)
{
    if (!pEngine->pAudioPlugin || !pEngine->pSkin)
        return S_OK;
    LPCSTR name = UixSkinGetAudioName(pEngine->pSkin, SoundResID);
    if (name)
        pEngine->pAudioPlugin->PlaySound(name);
    return S_OK;
}

HRESULT CUIXEngineInternal::LaunchDash(DWORD, DWORD, DWORD)
{
    return S_OK;   // extension features we host don't launch the dash
}

HRESULT CUIXEngineInternal::ShowPopup(LPCWSTR, DWORD, DWORD, DWORD, LPCWSTR)
{
    return S_OK;   // popup screens not needed by the keyboard/help extension
}

BOOL CUIXEngineInternal::CanRecordVoiceMailForPort(DWORD) { return FALSE; }
BOOL CUIXEngineInternal::CanPlayVoiceMailForPort(DWORD)   { return FALSE; }

HRESULT CUIXEngineInternal::ShowVoiceMailScreen(DWORD, XUID, LPCWSTR, LPCWSTR, BOOL,
                                                DWORD, DWORD, BYTE *)
{
    return E_NOTIMPL;
}

HRESULT CUIXEngineInternal::SetVoiceMailPlayScreenData(DWORD, DWORD, DWORD, BYTE *)
{
    return E_NOTIMPL;
}

// ---- extension-feature input (edge-triggered, per port) ----
enum {
    RK_A = 0, RK_B, RK_X, RK_Y, RK_START, RK_BACK, RK_BLACK, RK_WHITE,
    RK_LT, RK_RT, RK_DUP, RK_DDOWN, RK_DLEFT, RK_DRIGHT,
    RK_SUP, RK_SDOWN, RK_SLEFT, RK_SRIGHT, RK_COUNT
};
static const UIX_INPUT_TYPE g_richToUix[RK_COUNT] = {
    UIX_INPUT_A, UIX_INPUT_B, UIX_INPUT_X, UIX_INPUT_Y,
    UIX_INPUT_START, UIX_INPUT_BACK, UIX_INPUT_BLACK, UIX_INPUT_WHITE,
    UIX_INPUT_LEFT_TRIGGER, UIX_INPUT_RIGHT_TRIGGER,
    UIX_INPUT_DPAD_UP, UIX_INPUT_DPAD_DOWN, UIX_INPUT_DPAD_LEFT, UIX_INPUT_DPAD_RIGHT,
    UIX_INPUT_UP, UIX_INPUT_DOWN, UIX_INPUT_LEFT, UIX_INPUT_RIGHT
};
static DWORD DecodeRich(const XINPUT_STATE *pState)
{
    DWORD k = 0;
    const XINPUT_GAMEPAD *g = &pState->Gamepad;
    #define RB(idx, bit) do { if (g->bAnalogButtons[idx] > UIX_ANALOG_PRESS_THRESHOLD) k |= (1u << (bit)); } while (0)
    RB(XINPUT_GAMEPAD_A, RK_A); RB(XINPUT_GAMEPAD_B, RK_B);
    RB(XINPUT_GAMEPAD_X, RK_X); RB(XINPUT_GAMEPAD_Y, RK_Y);
    RB(XINPUT_GAMEPAD_BLACK, RK_BLACK); RB(XINPUT_GAMEPAD_WHITE, RK_WHITE);
    RB(XINPUT_GAMEPAD_LEFT_TRIGGER, RK_LT); RB(XINPUT_GAMEPAD_RIGHT_TRIGGER, RK_RT);
    #undef RB
    if (g->wButtons & XINPUT_GAMEPAD_START) k |= (1u << RK_START);
    if (g->wButtons & XINPUT_GAMEPAD_BACK)  k |= (1u << RK_BACK);
    if (g->wButtons & XINPUT_GAMEPAD_DPAD_UP)    k |= (1u << RK_DUP);
    if (g->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)  k |= (1u << RK_DDOWN);
    if (g->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)  k |= (1u << RK_DLEFT);
    if (g->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) k |= (1u << RK_DRIGHT);
    if (g->sThumbLY >  UIX_THUMB_DEADZONE) k |= (1u << RK_SUP);
    if (g->sThumbLY < -UIX_THUMB_DEADZONE) k |= (1u << RK_SDOWN);
    if (g->sThumbLX < -UIX_THUMB_DEADZONE) k |= (1u << RK_SLEFT);
    if (g->sThumbLX >  UIX_THUMB_DEADZONE) k |= (1u << RK_SRIGHT);
    return k;
}
static void PumpCustomInput(RXDK_UIX_ENGINE *e)
{
    for (DWORD port = 0; port < XONLINE_MAX_LOGON_USERS; port++) {
        DWORD pressed = e->fInputValid[port] ? DecodeRich(&e->input[port]) : 0;
        DWORD edges   = pressed & ~e->dwCustomPrevPressed[port];
        e->dwCustomPrevPressed[port] = pressed;
        if (!edges)
            continue;
        for (DWORD b = 0; b < RK_COUNT; b++) {
            if (!(edges & (1u << b)))
                continue;
            RXDK_UIX_SCREEN *top = TopScreen(e);   // re-fetch: Input() may push/pop screens
            if (!top || !top->pScreen || !top->fInputEnabled)
                break;
            top->pScreen->Input(port, g_richToUix[b]);
            if (e->fCustomEndRequested)
                return;   // feature asked to end; stop feeding input
        }
    }
}

// Tear down the active extension feature after it called EndFeature().
static void EndCustomFeature(RXDK_UIX_ENGINE *e)
{
    IUIXFeature *f = e->pCustomFeature;
    memset(&e->exitInfo, 0, sizeof(e->exitInfo));
    if (f)
        f->ReceiveMessage(UIX_FEATUREMSG_GET_EXIT_INFO, NULL, &e->exitInfo, NULL);
    e->exitInfo.FeatureID = e->customFeatureID;
    e->fExitPending = TRUE;
    if (f)
        f->HibernateFeature();   // deletes its screens -> DestroyScreen frees plugin objects
    for (DWORD i = 0; i < e->cScreens; i++)   // defensive: anything left registered
        if (e->screens[i].pScreen && e->pUIPlugin)
            e->pUIPlugin->DestroyScreenObjects(e->screens[i].dwInstance);
    e->cScreens            = 0;
    e->cScreenStack        = 0;
    e->pCustomFeature      = NULL;
    e->fCustomEndRequested = FALSE;
    e->state               = UIXST_EXIT_PENDING;
}

HRESULT WINAPI LiveEngine_DoWork(LiveEngine *pThis, DWORD *pDoWorkFlags)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    DWORD flags = 0;

    if (e->pUIPlugin && e->state != UIXST_IDLE)
        e->pUIPlugin->DoWork();
    if (e->pAudioPlugin)
        e->pAudioPlugin->DoWork();

    // background pumping: friends machinery always; the logon task whenever it
    // is not being completion-checked by the CONNECTING state below (keeping it
    // pumped is what holds the Live connection open)
    PumpTask(&e->hFriendsStartupTask);
    for (DWORD i = 0; i < XONLINE_MAX_LOGON_USERS; i++)
        PumpTask(&e->hFriendsEnumTask[i]);
    if (e->hLogonTask && e->state != UIXST_LOGON_CONNECTING)
        XOnlineTaskContinue(e->hLogonTask);

    // notifications: poll the friends snapshot a few times a second (works for
    // engine-driven and title-driven logon sessions alike)
    PXONLINE_USER pSessionUsers = XOnlineGetLogonUsers();
    if (pSessionUsers && e->dwProperties[UIX_PROPERTY_DISPLAY_NOTIFICATIONS]) {
        if (e->dwNotifyPollCountdown == 0) {
            e->dwNotifyPollCountdown = 30;
            for (DWORD u = 0; u < XONLINE_MAX_LOGON_USERS; u++) {
                DWORD nf = 0;
                if (pSessionUsers[u].xuid.qwUserID) {
                    EnsureFriendsMachinery(e, u);
                    if (XOnlineGetNotification(u, XONLINE_NOTIFICATION_FRIEND_REQUEST))
                        nf |= UIX_DOWORK_NOTIFY_FRIEND_REQUEST;
                    if (XOnlineGetNotification(u, XONLINE_NOTIFICATION_GAME_INVITE))
                        nf |= UIX_DOWORK_NOTIFY_GAME_INVITE;
                }
                e->dwNotifyFlags[u] = nf;
            }
        } else {
            e->dwNotifyPollCountdown--;
        }
        for (DWORD u = 0; u < XONLINE_MAX_LOGON_USERS; u++)
            if (e->dwNotifyFlags[u])
                flags |= UIX_DOWORK_NOTIFICATIONS | e->dwNotifyFlags[u];
    }

    switch (e->state) {

    case UIXST_LOGON_PICKING:
    {
        flags |= UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT;
        DWORD cRows = PickerRowCount(e);
        for (DWORD port = 0; port < XONLINE_MAX_LOGON_USERS; port++) {
            DWORD edges = PortEdges(e, port);
            if (!edges)
                continue;
            if (edges & UIXKEY_UP)
                e->iSelected = e->iSelected ? e->iSelected - 1 : cRows - 1;
            else if (edges & UIXKEY_DOWN)
                e->iSelected = (e->iSelected + 1) % cRows;
            else if (edges & UIXKEY_A)
                ClaimRow(e, port);
            else if (edges & UIXKEY_START) {
                if (e->cClaims)
                    StartLogonTask(e);
            } else if (edges & UIXKEY_B) {
                if (e->claims[port].iAccount >= 0)
                    UnclaimPort(e, port);
                else {
                    SetExit(e, _uix_logon_feature, UIX_EXIT_LOGON_USER_EXIT, S_OK, NULL);
                    break;
                }
            }
            if (e->state != UIXST_LOGON_PICKING)
                break;
            cRows = PickerRowCount(e);
            if (e->iSelected >= cRows)
                e->iSelected = cRows ? cRows - 1 : 0;
        }
        if (e->state == UIXST_EXIT_PENDING)
            flags = (flags & ~(UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT)) |
                    UIX_DOWORK_FEATURE_EXIT;
        break;
    }

    case UIXST_LOGON_PASSCODE:
    {
        flags |= UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT;
        DWORD edges = PortEdges(e, e->dwPasscodePort);
        if (edges & UIXKEY_B) {
            e->state = UIXST_LOGON_PICKING;
            break;
        }
        // A confirms once all digits entered
        if ((edges & UIXKEY_A) && e->cPasscodeEntered == XONLINE_PASSCODE_LENGTH) {
            if (memcmp(e->passcodeEntry, e->accounts[e->iPasscodeAccount].passcode,
                       XONLINE_PASSCODE_LENGTH) == 0) {
                DWORD port = e->dwPasscodePort;
                e->claims[port].iAccount = e->iPasscodeAccount;
                e->claims[port].fGuest   = FALSE;
                e->claimOrder[e->cClaims++] = port;
                e->state = UIXST_LOGON_PICKING;
                if (e->logonParams.LogonUserCount <= 1)
                    StartLogonTask(e);
            } else {
                e->fPasscodeError   = TRUE;
                e->cPasscodeEntered = 0;
            }
            break;
        }
        if (e->cPasscodeEntered < XONLINE_PASSCODE_LENGTH) {
            BYTE code = PasscodeCode(edges);
            if (code) {
                e->passcodeEntry[e->cPasscodeEntered++] = code;
                e->fPasscodeError = FALSE;
            }
        }
        break;
    }

    case UIXST_LOGON_CONNECTING:
    {
        flags |= UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT;
        for (DWORD port = 0; port < XONLINE_MAX_LOGON_USERS; port++) {
            DWORD edges = PortEdges(e, port);
            if (edges & UIXKEY_B) {
                XOnlineTaskClose(e->hLogonTask);
                e->hLogonTask = NULL;
                SetExit(e, _uix_logon_feature, UIX_EXIT_LOGON_USER_EXIT, S_OK, NULL);
            }
        }
        if (e->state != UIXST_LOGON_CONNECTING) {
            flags = UIX_DOWORK_FEATURE_EXIT;
            break;
        }

        HRESULT hr = XOnlineTaskContinue(e->hLogonTask);
        if (hr == XONLINETASK_S_RUNNING || hr == XONLINETASK_S_RUNNING_IDLE)
            break;
        if (hr == XONLINE_S_LOGON_CONNECTION_ESTABLISHED || SUCCEEDED(hr)) {
            e->fLoggedOn = TRUE;
            if (e->fInviteLogon) {
                // a cross-launch game-invite logon reports the invite through
                // the friends exit code (per the retail contract)
                e->exitFriend = e->inviteFriend;
                SetExit(e, _uix_logon_feature, UIX_EXIT_FRIENDS_JOIN_GAME, hr, &e->exitFriend);
            } else {
                SetExit(e, _uix_logon_feature, UIX_EXIT_LOGON_SUCCESSFUL, hr, &e->logonExitData);
            }
        } else {
            XOnlineTaskClose(e->hLogonTask);
            e->hLogonTask = NULL;
            SetExit(e, _uix_logon_feature, UIX_EXIT_LOGON_FAILED, hr, NULL);
        }
        flags = UIX_DOWORK_FEATURE_EXIT;
        break;
    }

    case UIXST_FRIENDS:
    {
        flags |= UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT;
        e->cFriendsSnap = XOnlineFriendsGetLatest(e->dwFriendsUser, MAX_FRIENDS, e->friendsSnap);
        if (e->iFriendSel >= e->cFriendsSnap && e->cFriendsSnap)
            e->iFriendSel = e->cFriendsSnap - 1;

        DWORD edges = PortEdges(e, e->dwFriendsPort);
        if (!edges)
            break;

        if (!e->fFriendMenuOpen) {
            if ((edges & UIXKEY_UP) && e->cFriendsSnap)
                e->iFriendSel = e->iFriendSel ? e->iFriendSel - 1 : e->cFriendsSnap - 1;
            else if ((edges & UIXKEY_DOWN) && e->cFriendsSnap)
                e->iFriendSel = (e->iFriendSel + 1) % e->cFriendsSnap;
            else if ((edges & UIXKEY_A) && e->cFriendsSnap) {
                e->fFriendMenuOpen = TRUE;
                e->iFriendMenuSel  = 0;
                e->wszStatus[0]    = 0;
            } else if (edges & UIXKEY_X) {
                if (e->fSignOutEnabled) {
                    XOnlineTaskClose(e->hLogonTask);
                    e->hLogonTask = NULL;
                    e->fLoggedOn  = FALSE;
                    SetExit(e, _uix_friends_feature, UIX_EXIT_FRIENDS_SIGNED_OUT, S_OK, NULL);
                }
            } else if (edges & UIXKEY_B) {
                SetExit(e, _uix_friends_feature, UIX_EXIT_FRIENDS_NORMAL_EXIT, S_OK, NULL);
            }
        } else {
            FRIEND_ACTION actions[FRMENU_MAX];
            const WCHAR *labels[FRMENU_MAX];
            const XONLINE_FRIEND *f = &e->friendsSnap[e->iFriendSel];
            DWORD cItems = BuildFriendMenu(e, f, actions, labels);
            if (e->iFriendMenuSel >= cItems)
                e->iFriendMenuSel = cItems - 1;

            if (edges & UIXKEY_UP)
                e->iFriendMenuSel = e->iFriendMenuSel ? e->iFriendMenuSel - 1 : cItems - 1;
            else if (edges & UIXKEY_DOWN)
                e->iFriendMenuSel = (e->iFriendMenuSel + 1) % cItems;
            else if (edges & UIXKEY_B)
                e->fFriendMenuOpen = FALSE;
            else if (edges & UIXKEY_A) {
                HRESULT hr;
                e->fFriendMenuOpen = FALSE;
                switch (actions[e->iFriendMenuSel]) {
                case FRACT_ACCEPT_REQUEST:
                    hr = XOnlineFriendsAnswerRequest(e->dwFriendsUser, f, XONLINE_REQUEST_YES);
                    FriendOpStatus(e, hr, L"Friend request accepted", L"Request answer failed");
                    break;
                case FRACT_DECLINE_REQUEST:
                    hr = XOnlineFriendsAnswerRequest(e->dwFriendsUser, f, XONLINE_REQUEST_NO);
                    FriendOpStatus(e, hr, L"Friend request declined", L"Request answer failed");
                    break;
                case FRACT_BLOCK_REQUEST:
                    hr = XOnlineFriendsAnswerRequest(e->dwFriendsUser, f, XONLINE_REQUEST_BLOCK);
                    FriendOpStatus(e, hr, L"Requests blocked", L"Request answer failed");
                    break;
                case FRACT_ACCEPT_INVITE:
                    hr = XOnlineFriendsAnswerGameInvite(e->dwFriendsUser, f, XONLINE_GAMEINVITE_YES);
                    if (SUCCEEDED(hr)) {
                        e->exitFriend = *f;
                        SetExit(e, _uix_friends_feature,
                                XOnlineTitleIdIsSameTitle(f->dwTitleID)
                                    ? UIX_EXIT_FRIENDS_JOIN_GAME
                                    : UIX_EXIT_FRIENDS_JOIN_GAME_CROSS_TITLE,
                                hr, &e->exitFriend);
                    } else {
                        FriendOpStatus(e, hr, L"", L"Invite accept failed");
                    }
                    break;
                case FRACT_DECLINE_INVITE:
                    hr = XOnlineFriendsAnswerGameInvite(e->dwFriendsUser, f, XONLINE_GAMEINVITE_NO);
                    FriendOpStatus(e, hr, L"Game invite declined", L"Invite answer failed");
                    break;
                case FRACT_JOIN:
                    e->exitFriend = *f;
                    SetExit(e, _uix_friends_feature, UIX_EXIT_FRIENDS_JOIN_GAME, S_OK, &e->exitFriend);
                    break;
                case FRACT_INVITE:
                    hr = XOnlineFriendsGameInvite(e->dwFriendsUser, e->sessionID, 1, f);
                    FriendOpStatus(e, hr, L"Game invite sent", L"Game invite failed");
                    break;
                case FRACT_REMOVE:
                    hr = XOnlineFriendsRemove(e->dwFriendsUser, f);
                    FriendOpStatus(e, hr, L"Friend removed", L"Remove failed");
                    break;
                default:
                    break;
                }
            }
        }
        if (e->state == UIXST_EXIT_PENDING)
            flags = (flags & ~(UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT)) |
                    UIX_DOWORK_FEATURE_EXIT;
        break;
    }

    case UIXST_PLAYERS:
    {
        flags |= UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT;
        BuildPlayersView(e);
        if (e->iPlayerSel >= e->cView && e->cView)
            e->iPlayerSel = e->cView - 1;

        for (DWORD port = 0; port < XONLINE_MAX_LOGON_USERS; port++) {
            if (e->dwPlayersPort != UIX_INVALID_VALUE && port != e->dwPlayersPort) {
                PortEdges(e, port); // keep edge state coherent
                continue;
            }
            DWORD edges = PortEdges(e, port);
            if (!edges)
                continue;

            if (!e->fPlayerMenuOpen) {
                if ((edges & UIXKEY_UP) && e->cView)
                    e->iPlayerSel = e->iPlayerSel ? e->iPlayerSel - 1 : e->cView - 1;
                else if ((edges & UIXKEY_DOWN) && e->cView)
                    e->iPlayerSel = (e->iPlayerSel + 1) % e->cView;
                else if ((edges & UIXKEY_A) && e->cView) {
                    const ITitlePlayersListItem *p = e->view[e->iPlayerSel];
                    // the controlling player's own row is highlight-only
                    if (p != e->playersParams.pPlayerControllingScreen) {
                        e->fPlayerMenuOpen = TRUE;
                        e->iPlayerMenuSel  = 0;
                        e->wszStatus[0]    = 0;
                    }
                } else if (edges & UIXKEY_B) {
                    SetExit(e, _uix_players_feature, UIX_EXIT_PLAYERS_NORMAL_EXIT, S_OK, NULL);
                    break;
                }
            } else {
                // menu: Mute / Unmute / Cancel
                const DWORD cItems = 3;
                if (edges & UIXKEY_UP)
                    e->iPlayerMenuSel = e->iPlayerMenuSel ? e->iPlayerMenuSel - 1 : cItems - 1;
                else if (edges & UIXKEY_DOWN)
                    e->iPlayerMenuSel = (e->iPlayerMenuSel + 1) % cItems;
                else if (edges & UIXKEY_B)
                    e->fPlayerMenuOpen = FALSE;
                else if (edges & UIXKEY_A) {
                    e->fPlayerMenuOpen = FALSE;
                    const ITitlePlayersListItem *p = e->view[e->iPlayerSel];
                    const XUID *px = ((ITitlePlayersListItem *)p)->GetXUID();
                    DWORD userIdx = (e->dwPlayersPort != UIX_INVALID_VALUE) ? e->dwPlayersPort : 0;
                    if (e->iPlayerMenuSel == 0 && px) {
                        HRESULT hr = XOnlineMutelistAdd(userIdx, *px);
                        FriendOpStatus(e, hr, L"Player muted", L"Mute list unavailable");
                    } else if (e->iPlayerMenuSel == 1 && px) {
                        HRESULT hr = XOnlineMutelistRemove(userIdx, *px);
                        FriendOpStatus(e, hr, L"Player unmuted", L"Mute list unavailable");
                    }
                }
            }
        }

        // publish the current selection
        memset(&e->selInfo, 0, sizeof(e->selInfo));
        e->selInfo.FeatureID = _uix_players_feature;
        if (e->cView) {
            e->selInfo.pSelectedCurrentPlayer = (ITitlePlayersListItem *)e->view[e->iPlayerSel];
            e->selInfo.CanDisplay = TRUE;
        }

        if (e->state == UIXST_EXIT_PENDING)
            flags = (flags & ~(UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT)) |
                    UIX_DOWORK_FEATURE_EXIT;
        break;
    }

    case UIXST_CUSTOM:
        flags |= UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT;
        if (e->pCustomFeature)
            e->pCustomFeature->PumpTasks();
        PumpCustomInput(e);
        if (e->fCustomEndRequested) {
            EndCustomFeature(e);   // -> state UIXST_EXIT_PENDING
            flags = (flags & ~(UIX_DOWORK_NEED_TO_RENDER | UIX_DOWORK_PROCESSING_INPUT))
                  | UIX_DOWORK_FEATURE_EXIT;
        }
        break;

    case UIXST_EXIT_PENDING:
        flags |= UIX_DOWORK_FEATURE_EXIT;
        break;

    case UIXST_IDLE:
    default:
        break;
    }

    if (pDoWorkFlags)
        *pDoWorkFlags = flags;
    return S_OK;
}

// ------------------------------------------------------------- rendering ---

static void RenderHeader(ITitleFontRenderer *pFont, const WCHAR *pSub, DWORD X, DWORD *pY, DWORD W)
{
    pFont->SetHeight(28);
    pFont->SetColor(0xFFFFFFFF);
    pFont->DrawText(L"Xbox Live", X, *pY, W);
    *pY += 44;
    if (pSub) {
        pFont->SetHeight(20);
        pFont->SetColor(0xFFC0C0C0);
        pFont->DrawText(pSub, X, *pY, W);
        *pY += 32;
    } else {
        pFont->SetHeight(20);
    }
}

HRESULT WINAPI LiveEngine_Render(LiveEngine *pThis, IDirect3DSurface8 *pSurface)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    if (e->state == UIXST_IDLE || e->state == UIXST_EXIT_PENDING)
        return S_OK;
    if (!e->pUIPlugin)
        return S_OK;

    e->pUIPlugin->SetRenderTarget(pSurface);
    ITitleFontRenderer *pFont = NULL;
    if (FAILED(e->pUIPlugin->GetFont(&pFont)) || !pFont)
        return S_OK;

    const DWORD X = 96, W = 448;
    DWORD y = 80;
    WCHAR wszLine[XONLINE_GAMERTAG_SIZE + 48];

    switch (e->state) {

    case UIXST_LOGON_PICKING:
    {
        RenderHeader(pFont, L"Select accounts", X, &y, W);
        DWORD cRows = PickerRowCount(e);
        const DWORD ROWS = 8;
        DWORD first = 0;
        if (cRows > ROWS && e->iSelected >= ROWS / 2) {
            first = e->iSelected - ROWS / 2;
            if (first + ROWS > cRows)
                first = cRows - ROWS;
        }
        for (DWORD i = first; i < cRows && i < first + ROWS; i++) {
            wszLine[0] = 0;
            if (i < e->cAccounts) {
                WCHAR wszTag[XONLINE_GAMERTAG_SIZE];
                WidenTag(e->accounts[i].szGamertag, wszTag, XONLINE_GAMERTAG_SIZE);
                AppendW(wszLine, 64, wszTag);
                DWORD port;
                if (AccountClaimed(e, (LONG)i, &port)) {
                    const WCHAR *marks[4] = { L"  [P1]", L"  [P2]", L"  [P3]", L"  [P4]" };
                    AppendW(wszLine, 64, marks[port & 3]);
                }
            } else {
                LONG iAccount = GuestRowAccount(e, i - e->cAccounts);
                WCHAR wszTag[XONLINE_GAMERTAG_SIZE];
                WidenTag(e->accounts[iAccount >= 0 ? iAccount : 0].szGamertag, wszTag,
                         XONLINE_GAMERTAG_SIZE);
                AppendW(wszLine, 64, L"Guest of ");
                AppendW(wszLine, 64, wszTag);
            }
            if (i == e->iSelected) {
                pFont->SetColor(0xFF00FF00);
                pFont->DrawText(L">", X, y, 16);
            } else {
                pFont->SetColor(0xFFFFFFFF);
            }
            pFont->DrawText(wszLine, X + 24, y, W - 24);
            y += 26;
        }
        y += 12;
        pFont->SetColor(0xFF808080);
        pFont->DrawText(e->logonParams.LogonUserCount > 1
                            ? L"A Select    START Sign in    B Cancel"
                            : L"A Select    B Cancel",
                        X, y, W);
        break;
    }

    case UIXST_LOGON_PASSCODE:
    {
        WCHAR wszTag[XONLINE_GAMERTAG_SIZE];
        WidenTag(e->accounts[e->iPasscodeAccount].szGamertag, wszTag, XONLINE_GAMERTAG_SIZE);
        wszLine[0] = 0;
        AppendW(wszLine, 64, L"Passcode for ");
        AppendW(wszLine, 64, wszTag);
        RenderHeader(pFont, wszLine, X, &y, W);

        WCHAR wszDots[XONLINE_PASSCODE_LENGTH * 2 + 1];
        for (DWORD i = 0; i < XONLINE_PASSCODE_LENGTH; i++) {
            wszDots[i * 2]     = (i < e->cPasscodeEntered) ? L'*' : L'_';
            wszDots[i * 2 + 1] = L' ';
        }
        wszDots[XONLINE_PASSCODE_LENGTH * 2] = 0;
        pFont->SetColor(0xFFFFFFFF);
        pFont->DrawText(wszDots, X + 16, y, W);
        y += 32;
        if (e->fPasscodeError) {
            pFont->SetColor(0xFFFF4040);
            pFont->DrawText(L"Incorrect passcode", X, y, W);
            y += 26;
        }
        pFont->SetColor(0xFF808080);
        pFont->DrawText(L"DPAD / X / Y / triggers enter    A OK    B Back", X, y, W);
        break;
    }

    case UIXST_LOGON_CONNECTING:
    {
        RenderHeader(pFont, L"Connecting to Xbox Live as", X, &y, W);
        for (DWORD k = 0; k < (e->cClaims ? e->cClaims : 1); k++) {
            const XONLINE_USER *u = &e->logonUsers[k];
            if (!u->xuid.qwUserID && k)
                break;
            WCHAR wszTag[XONLINE_GAMERTAG_SIZE];
            WidenTag(u->szGamertag, wszTag, XONLINE_GAMERTAG_SIZE);
            pFont->SetColor(0xFFFFFFFF);
            pFont->DrawText(wszTag, X + 16, y, W - 16);
            y += 26;
        }
        y += 14;
        pFont->SetColor(0xFF808080);
        pFont->DrawText(L"B Cancel", X, y, W);
        break;
    }

    case UIXST_FRIENDS:
    {
        RenderHeader(pFont, L"Friends", X, &y, W);
        if (e->cFriendsSnap == 0) {
            pFont->SetColor(0xFFC0C0C0);
            pFont->DrawText(L"No friends yet", X, y, W);
            y += 26;
        }
        const DWORD ROWS = 7;
        DWORD first = 0;
        if (e->cFriendsSnap > ROWS && e->iFriendSel >= ROWS / 2) {
            first = e->iFriendSel - ROWS / 2;
            if (first + ROWS > e->cFriendsSnap)
                first = e->cFriendsSnap - ROWS;
        }
        for (DWORD i = first; i < e->cFriendsSnap && i < first + ROWS; i++) {
            const XONLINE_FRIEND *f = &e->friendsSnap[i];
            WCHAR wszTag[XONLINE_GAMERTAG_SIZE];
            WidenTag(f->szGamertag, wszTag, XONLINE_GAMERTAG_SIZE);
            wszLine[0] = 0;
            AppendW(wszLine, 64, wszTag);
            DWORD s = f->dwFriendState;
            if (s & XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST)
                AppendW(wszLine, 64, L"  [request]");
            if (s & XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE)
                AppendW(wszLine, 64, L"  [invite]");
            else if (s & XONLINE_FRIENDSTATE_FLAG_JOINABLE)
                AppendW(wszLine, 64, L"  [joinable]");
            else if (s & XONLINE_FRIENDSTATE_FLAG_PLAYING)
                AppendW(wszLine, 64, L"  [playing]");
            else if (s & XONLINE_FRIENDSTATE_FLAG_ONLINE)
                AppendW(wszLine, 64, L"  [online]");
            if (i == e->iFriendSel) {
                pFont->SetColor(0xFF00FF00);
                pFont->DrawText(L">", X, y, 16);
            } else {
                pFont->SetColor(0xFFFFFFFF);
            }
            pFont->DrawText(wszLine, X + 24, y, W - 24);
            y += 26;
        }

        if (e->fFriendMenuOpen && e->cFriendsSnap) {
            FRIEND_ACTION actions[FRMENU_MAX];
            const WCHAR *labels[FRMENU_MAX];
            DWORD cItems = BuildFriendMenu(e, &e->friendsSnap[e->iFriendSel], actions, labels);
            y += 8;
            for (DWORD i = 0; i < cItems; i++) {
                pFont->SetColor(i == e->iFriendMenuSel ? 0xFF00FF00 : 0xFFFFFFFF);
                pFont->DrawText(labels[i], X + 48, y, W - 48);
                y += 24;
            }
        } else {
            y += 12;
            pFont->SetColor(0xFF808080);
            pFont->DrawText(e->fSignOutEnabled ? L"A Options    X Sign out    B Back"
                                               : L"A Options    B Back",
                            X, y, W);
            y += 24;
        }
        if (e->wszStatus[0]) {
            pFont->SetColor(0xFFFFFF80);
            pFont->DrawText(e->wszStatus, X, y, W);
        }
        break;
    }

    case UIXST_PLAYERS:
    {
        RenderHeader(pFont,
                     (e->playersParams.DisplayType & UIX_PLAYERS_DISPLAY_DEPARTED_PLAYERS)
                         ? L"Players (departed)"
                         : L"Players",
                     X, &y, W);
        if (e->cView == 0) {
            pFont->SetColor(0xFFC0C0C0);
            pFont->DrawText(L"No players", X, y, W);
            y += 26;
        }
        const DWORD ROWS = 8;
        DWORD first = 0;
        if (e->cView > ROWS && e->iPlayerSel >= ROWS / 2) {
            first = e->iPlayerSel - ROWS / 2;
            if (first + ROWS > e->cView)
                first = e->cView - ROWS;
        }
        for (DWORD i = first; i < e->cView && i < first + ROWS; i++) {
            ITitlePlayersListItem *p = (ITitlePlayersListItem *)e->view[i];
            const WCHAR *pName = p->GetName();
            wszLine[0] = 0;
            AppendW(wszLine, 64, pName ? pName : L"?");
            switch (p->GetVoiceStatus()) {
            case UIX_VOICE_STATUS_COMMUNICATOR:
                AppendW(wszLine, 64, p->IsTalking() ? L"  [talking]" : L"  [voice]");
                break;
            case UIX_VOICE_STATUS_SPEAKERS:
                AppendW(wszLine, 64, L"  [speakers]");
                break;
            default:
                break;
            }
            if (i == e->iPlayerSel) {
                pFont->SetColor(0xFF00FF00);
                pFont->DrawText(L">", X, y, 16);
            } else {
                pFont->SetColor(0xFFFFFFFF);
            }
            pFont->DrawText(wszLine, X + 24, y, W - 24);
            y += 26;
        }

        if (e->fPlayerMenuOpen) {
            static const WCHAR *menu[3] = { L"Mute", L"Unmute", L"Cancel" };
            y += 8;
            for (DWORD i = 0; i < 3; i++) {
                pFont->SetColor(i == e->iPlayerMenuSel ? 0xFF00FF00 : 0xFFFFFFFF);
                pFont->DrawText(menu[i], X + 48, y, W - 48);
                y += 24;
            }
        } else {
            y += 12;
            pFont->SetColor(0xFF808080);
            pFont->DrawText(L"A Options    B Back", X, y, W);
            y += 24;
        }
        if (e->wszStatus[0]) {
            pFont->SetColor(0xFFFFFF80);
            pFont->DrawText(e->wszStatus, X, y, W);
        }
        break;
    }

    case UIXST_CUSTOM:
        // Extension feature: render its shown screens bottom-to-top. Each screen
        // draws its own objects through the title's plugin (render target set above).
        for (DWORD i = 0; i < e->cScreenStack; i++) {
            RXDK_UIX_SCREEN *s = ScreenRec(e, e->screenStack[i]);
            if (s && s->pScreen)
                s->pScreen->Output();
        }
        break;

    default:
        break;
    }
    e->pUIPlugin->SetRenderTarget(NULL);   // end the render pass (plugin restores state)
    return S_OK;
}

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
    e->state = UIXST_IDLE;
    return S_OK;
}

HRESULT WINAPI LiveEngine_EndFeature(LiveEngine *pThis)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    switch (e->state) {
    case UIXST_LOGON_PICKING:
    case UIXST_LOGON_PASSCODE:
    case UIXST_LOGON_CONNECTING:
        if (e->hLogonTask && !e->fLoggedOn) {
            XOnlineTaskClose(e->hLogonTask);
            e->hLogonTask = NULL;
        }
        // fall through
    case UIXST_FRIENDS:
    case UIXST_PLAYERS:
        e->state = UIXST_IDLE;
        e->fExitPending = FALSE;
        break;
    case UIXST_CUSTOM:
        // Called from within the feature's screen Input() (mid-DoWork). Defer the
        // teardown to the DoWork UIXST_CUSTOM case so the screen stack isn't torn
        // down underneath the running Input handler.
        e->fCustomEndRequested = TRUE;
        break;
    default:
        break;
    }
    return S_OK;
}

HRESULT WINAPI LiveEngine_LogOff(LiveEngine *pThis)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    for (DWORD i = 0; i < XONLINE_MAX_LOGON_USERS; i++) {
        if (e->hFriendsEnumTask[i]) {
            XOnlineFriendsEnumerateFinish(e->hFriendsEnumTask[i]);
            e->hFriendsEnumTask[i] = NULL;
        }
    }
    if (e->hFriendsStartupTask) {
        XOnlineTaskClose(e->hFriendsStartupTask);
        e->hFriendsStartupTask = NULL;
    }
    if (e->hLogonTask) {
        XOnlineTaskClose(e->hLogonTask);
        e->hLogonTask = NULL;
    }
    e->fLoggedOn = FALSE;
    memset(e->logonUsers, 0, sizeof(e->logonUsers));
    memset(e->dwNotifyFlags, 0, sizeof(e->dwNotifyFlags));
    if (e->state != UIXST_EXIT_PENDING)
        e->state = UIXST_IDLE;
    return S_OK;
}

HRESULT WINAPI LiveEngine_GetFriendsForUser(LiveEngine *pThis, DWORD Port,
                                            ILiveFriendsList **ppFriendsList)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    if (!ppFriendsList)
        return E_POINTER;
    *ppFriendsList = NULL;
    if (Port >= XONLINE_MAX_LOGON_USERS)
        return E_INVALIDARG;
    if (!XOnlineGetLogonUsers())
        return E_FAIL; // needs an established Live session (engine- or title-driven)
    EnsureFriendsMachinery(e, Port);
    *ppFriendsList = (ILiveFriendsList *)&e->friendsLists[Port];
    return S_OK;
}

HRESULT WINAPI LiveEngine_NotificationSetState(LiveEngine *pThis, DWORD Port, DWORD StateFlags,
                                               XNKID SessionID, DWORD StateDataSize,
                                               const PVOID pStateData)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    // remember the published session so "Invite to game" can target it
    e->sessionID    = SessionID;
    e->fHaveSession = TRUE;
    for (DWORD i = 0; i < sizeof(XNKID); i++)
        if (((const BYTE *)&SessionID)[i])
            goto have_session;
    e->fHaveSession = FALSE;
have_session:
    return XOnlineNotificationSetState(Port, StateFlags, SessionID, StateDataSize,
                                       (const BYTE *)pStateData);
}

HRESULT WINAPI LiveEngine_GetNotifications(LiveEngine *pThis, DWORD Port,
                                           UIX_NOTIFICATION_PURPOSE Purpose, DWORD *pNotifications)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    (void)Purpose; // menu and in-game-flash surface the same pending set
    if (!pNotifications)
        return E_POINTER;
    if (Port >= XONLINE_MAX_LOGON_USERS)
        return E_INVALIDARG;
    *pNotifications = e->dwNotifyFlags[Port];
    return S_OK;
}

HRESULT WINAPI LiveEngine_SetProperty(LiveEngine *pThis, UIX_PROPERTY_TYPE Property, DWORD Value)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    if ((DWORD)Property >= 6)
        return E_INVALIDARG;
    e->dwProperties[Property] = Value;
    return S_OK;
}

HRESULT WINAPI LiveEngine_GetProperty(LiveEngine *pThis, UIX_PROPERTY_TYPE Property, DWORD *pValue)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    if (!pValue)
        return E_POINTER;
    if ((DWORD)Property >= 6)
        return E_INVALIDARG;
    *pValue = e->dwProperties[Property];
    return S_OK;
}

// launch the dashboard with the given context (account signup, network config,
// account management ... the XLD_LAUNCH_DASHBOARD_* reasons)
HRESULT WINAPI LiveEngine_Reboot(LiveEngine *pThis, DWORD Context)
{
    (void)pThis;
    LD_LAUNCH_DASHBOARD ld;
    memset(&ld, 0, sizeof(ld));
    ld.dwReason = Context;
    XLaunchNewImage(NULL, (PLAUNCH_DATA)&ld);
    return E_FAIL; // only reached if the launch failed
}

HRESULT WINAPI LiveEngine_GetFeatureInterface(LiveEngine *pThis, UIX_FEATURE FeatureID,
                                              const VOID *pParam, VOID **ppFeatureInterface)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    (void)pParam;
    if (!ppFeatureInterface)
        return E_POINTER;
    *ppFeatureInterface = NULL;
    if (FeatureID == _uix_players_feature) {
        *ppFeatureInterface = (ILivePlayersList *)&e->playersList;
        return S_OK;
    }
    return E_FAIL; // no other feature publishes an interface
}

HRESULT WINAPI LiveEngine_GetSelectionInfo(LiveEngine *pThis, UIX_SELECTION_INFO *pSelectionInfo)
{
    RXDK_UIX_ENGINE *e = Eng(pThis);
    if (!pSelectionInfo)
        return E_POINTER;
    if (e->state == UIXST_FRIENDS && e->cFriendsSnap) {
        memset(pSelectionInfo, 0, sizeof(*pSelectionInfo));
        pSelectionInfo->FeatureID      = _uix_friends_feature;
        pSelectionInfo->SelectedFriend = e->friendsSnap[e->iFriendSel];
        pSelectionInfo->CanDisplay     = TRUE;
        return S_OK;
    }
    if (e->selInfo.FeatureID) {
        *pSelectionInfo = e->selInfo;
        return S_OK;
    }
    memset(pSelectionInfo, 0, sizeof(*pSelectionInfo));
    return E_FAIL;
}

HRESULT WINAPI LiveEngine_UseVoiceMail(LiveEngine *pThis, UIX_VOICE_MAIL_ENTRY_POINT VoiceMailEntryPoint)
{
    // Capability boundary shared with libxvoice: voicemail records/plays through
    // the WMAVoice codec, which only ever existed as binaries inside the retail
    // libs -- with no codec the voice-mail UI cannot function, so entry is
    // refused the same way retail refuses it without a communicator.
    (void)pThis;
    (void)VoiceMailEntryPoint;
    return E_FAIL;
}

// -------------------------------------------------------- ILiveFriendsList ---
// Engine-owned per-user objects over the leak's live friends snapshot.

typedef RXDK_UIX_ENGINE::FRIENDS_LIST_OBJ FRIENDS_LIST_OBJ;

static FRIENDS_LIST_OBJ *Fl(LiveFriendsList *pThis)
{
    return (FRIENDS_LIST_OBJ *)pThis;
}

ULONG WINAPI LiveFriendsList_AddRef(LiveFriendsList *pThis)
{
    (void)pThis;
    return 2; // engine-owned; lifetime is the engine's
}

ULONG WINAPI LiveFriendsList_Release(LiveFriendsList *pThis)
{
    (void)pThis;
    return 1;
}

HRESULT WINAPI LiveFriendsList_Refresh(LiveFriendsList *pThis)
{
    FRIENDS_LIST_OBJ *fl = Fl(pThis);
    EnsureFriendsMachinery(fl->pEngine, fl->dwUserIndex);
    return S_OK;
}

BOOL WINAPI LiveFriendsList_IsReady(LiveFriendsList *pThis)
{
    FRIENDS_LIST_OBJ *fl = Fl(pThis);
    return fl->pEngine->hFriendsEnumTask[fl->dwUserIndex] != NULL;
}

BOOL WINAPI LiveFriendsList_HasChanged(LiveFriendsList *pThis)
{
    FRIENDS_LIST_OBJ *fl = Fl(pThis);
    static XONLINE_FRIEND snap[MAX_FRIENDS];
    DWORD c = XOnlineFriendsGetLatest(fl->dwUserIndex, MAX_FRIENDS, snap);
    return FriendsSnapshotStamp(snap, c) != fl->dwConsumedStamp;
}

DWORD WINAPI LiveFriendsList_Count(LiveFriendsList *pThis)
{
    FRIENDS_LIST_OBJ *fl = Fl(pThis);
    static XONLINE_FRIEND snap[MAX_FRIENDS];
    DWORD c = XOnlineFriendsGetLatest(fl->dwUserIndex, MAX_FRIENDS, snap);
    fl->dwConsumedStamp = FriendsSnapshotStamp(snap, c);
    return c;
}

HRESULT WINAPI LiveFriendsList_GetFriendByIndex(LiveFriendsList *pThis, DWORD FriendIndex,
                                                XONLINE_FRIEND *pFriend)
{
    FRIENDS_LIST_OBJ *fl = Fl(pThis);
    if (!pFriend)
        return E_POINTER;
    static XONLINE_FRIEND snap[MAX_FRIENDS];
    DWORD c = XOnlineFriendsGetLatest(fl->dwUserIndex, MAX_FRIENDS, snap);
    fl->dwConsumedStamp = FriendsSnapshotStamp(snap, c);
    if (FriendIndex >= c)
        return E_INVALIDARG;
    *pFriend = snap[FriendIndex];
    return S_OK;
}

HRESULT WINAPI LiveFriendsList_GetFriendByXUID(LiveFriendsList *pThis, XUID FriendXUID,
                                               XONLINE_FRIEND *pFriend)
{
    FRIENDS_LIST_OBJ *fl = Fl(pThis);
    if (!pFriend)
        return E_POINTER;
    static XONLINE_FRIEND snap[MAX_FRIENDS];
    DWORD c = XOnlineFriendsGetLatest(fl->dwUserIndex, MAX_FRIENDS, snap);
    fl->dwConsumedStamp = FriendsSnapshotStamp(snap, c);
    for (DWORD i = 0; i < c; i++) {
        if (snap[i].xuid.qwUserID == FriendXUID.qwUserID) {
            *pFriend = snap[i];
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT WINAPI LiveFriendsList_Remove(LiveFriendsList *pThis, const XONLINE_FRIEND *pFriend)
{
    FRIENDS_LIST_OBJ *fl = Fl(pThis);
    if (!pFriend)
        return E_POINTER;
    return XOnlineFriendsRemove(fl->dwUserIndex, pFriend);
}

HRESULT WINAPI LiveFriendsList_Request(LiveFriendsList *pThis, XUID UserXUID)
{
    FRIENDS_LIST_OBJ *fl = Fl(pThis);
    return XOnlineFriendsRequest(fl->dwUserIndex, UserXUID);
}

// ---------------------------------------------------------------- plugin ---
// PluginSupport (the skin-resource access interface) is now implemented in
// uix_skin.cpp, backed by the loaded .uix skin. Extension features (UIXKeyboard)
// resolve real strings/layouts/images/audio through it.

} // extern "C"
