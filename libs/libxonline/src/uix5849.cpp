// RXDK 5849 uplift: the UIX "Drop-In UI" Live engine (uix.h / uix.lib), which the
// 5849 Live samples use for the logon / friends / players screens. The leak has
// no UIX source at all (uix.lib shipped binary-only), so like the stats/storage
// entry points in uplift5849.cpp these are documented STUBS: creation succeeds
// and returns a real refcounted engine object so titles initialize and run their
// frame loop (DoWork/Render/SetInput all succeed doing nothing), but no UI is
// ever drawn and no feature (logon/friends/players) ever starts or completes --
// the same dead-air a retail box shows with no Live connection. Must be
// implemented for real Live use (e.g. Insignia).
// TODO(5849-uix): implement the drop-in UI over the CXo logon session.
//
// uix.lib is folded into libxonline (see docs/5849-uplift.md): every Live sample
// already links libxonline, and UIX is the UI face of the same online stack.

#include <xtl.h>
#include <xobjbase.h> // DECLARE_INTERFACE/THIS_/PURE for uix.h's title-plugin vtables
#include <d3d8.h>
#include <xonline.h>
#include <uix.h>

#include <stdlib.h> // malloc/free
#include <string.h> // memset

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

// ----------------------------------------------------------------- engine ---
// The public LiveEngine is opaque (uix.h forward-declares it and routes every
// method through the exported C functions), so the layout is ours.
struct RXDK_UIX_ENGINE
{
    LONG              lRefCount;
    ITitleUIPlugin   *pUIPlugin;
    ITitleAudioPlugin*pAudioPlugin;
};

static RXDK_UIX_ENGINE *Eng(LiveEngine *pThis)
{
    return (RXDK_UIX_ENGINE *)pThis;
}

// The default UI plugin UIXCreateUIPlugin hands back is only ever passed on to
// LiveEngine_SetUIPlugin, so an opaque non-NULL token satisfies titles.
static int s_defaultUIPlugin;

extern "C" {

XBOXAPI HRESULT WINAPI UIXCreateLiveEngine(LPCSTR pSkinFileName, DWORD LanguageID,
                                           ILiveEngine **ppEngine)
{
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
    *ppEngine = (ILiveEngine *)e;
    return S_OK;
}

XBOXAPI HRESULT WINAPI UIXCreateUIPlugin(ITitleFontRenderer *pFont, ITitleUIPlugin **ppUIPlugin)
{
    (void)pFont;
    if (!ppUIPlugin)
        return E_POINTER;
    *ppUIPlugin = (ITitleUIPlugin *)&s_defaultUIPlugin;
    return S_OK;
}

// Same deal for the default audio plugin (built over the title's XACT engine):
// only ever handed back to LiveEngine_SetAudioPlugin.
static int s_defaultAudioPlugin;

XBOXAPI HRESULT WINAPI UIXCreateAudioPlugin(VOID *pXactEngine, VOID *pXactSoundBank,
                                            ITitleAudioPlugin **ppAudioPlugin)
{
    (void)pXactEngine;
    (void)pXactSoundBank;
    if (!ppAudioPlugin)
        return E_POINTER;
    *ppAudioPlugin = (ITitleAudioPlugin *)&s_defaultAudioPlugin;
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
        free(e);
        return 0;
    }
    return (ULONG)cRef;
}

// Frame pump: nothing ever needs rendering, no feature ever exits.
HRESULT WINAPI LiveEngine_DoWork(LiveEngine *pThis, DWORD *pDoWorkFlags)
{
    (void)pThis;
    if (pDoWorkFlags)
        *pDoWorkFlags = 0;
    return S_OK;
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
    (void)pThis;
    (void)FeatureID;
    return S_OK;
}

// No feature can actually run without the UIX UI implementation; report failure
// so titles never wait on a logon/friends screen that cannot appear.
HRESULT WINAPI LiveEngine_StartFeature(LiveEngine *pThis, UIX_FEATURE FeatureID,
                                       const VOID *pFeatureParams)
{
    (void)pThis;
    (void)FeatureID;
    (void)pFeatureParams;
    return E_FAIL;
}

HRESULT WINAPI LiveEngine_Render(LiveEngine *pThis, IDirect3DSurface8 *pSurface)
{
    (void)pThis;
    (void)pSurface;
    return S_OK; // nothing to draw
}

HRESULT WINAPI LiveEngine_SetInput(LiveEngine *pThis, DWORD Port, const XINPUT_STATE *pInputState)
{
    (void)pThis;
    (void)Port;
    (void)pInputState;
    return S_OK;
}

HRESULT WINAPI LiveEngine_GetFriendsForUser(LiveEngine *pThis, DWORD Port,
                                            ILiveFriendsList **ppFriendsList)
{
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
    (void)pThis;
    (void)Port;
    (void)StateFlags;
    (void)SessionID;
    (void)StateDataSize;
    (void)pStateData;
    return S_OK;
}

HRESULT WINAPI LiveEngine_GetNotifications(LiveEngine *pThis, DWORD Port,
                                           UIX_NOTIFICATION_PURPOSE Purpose, DWORD *pNotifications)
{
    (void)pThis;
    (void)Port;
    (void)Purpose;
    if (!pNotifications)
        return E_POINTER;
    *pNotifications = 0;
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

// No feature ever starts, so none can have exited.
HRESULT WINAPI LiveEngine_GetExitInfo(LiveEngine *pThis, UIX_EXIT_INFO *pExitInfo)
{
    (void)pThis;
    if (pExitInfo)
        memset(pExitInfo, 0, sizeof(*pExitInfo));
    return E_FAIL;
}

HRESULT WINAPI LiveEngine_Reboot(LiveEngine *pThis, DWORD Context)
{
    (void)pThis;
    (void)Context;
    return E_FAIL; // dashboard reboot contexts need the real UIX/dash handshake
}

HRESULT WINAPI LiveEngine_LogOff(LiveEngine *pThis)
{
    (void)pThis;
    return S_OK;
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
    return E_FAIL;
}

HRESULT WINAPI LiveEngine_EndFeature(LiveEngine *pThis)
{
    (void)pThis;
    return S_OK;
}

HRESULT WINAPI LiveEngine_ClearInput(LiveEngine *pThis)
{
    (void)pThis;
    return S_OK;
}

HRESULT WINAPI LiveEngine_UseVoiceMail(LiveEngine *pThis, UIX_VOICE_MAIL_ENTRY_POINT VoiceMailEntryPoint)
{
    (void)pThis;
    (void)VoiceMailEntryPoint;
    return E_FAIL; // voicemail needs the WMAVoice codec + UIX voice-mail UI
}

} // extern "C"
