// RXDK 5849 uplift: exports the XACT engine APIs that the imported 5849 samples reference but that
// are shaped/named differently in (or absent from) the Jan-2002 source leak.
//
//   * PrepareEx -- 5849 turned SoundBank Play into an inline over PrepareEx. The leak has no
//     "prepare a cue without playing" path, so PrepareEx forwards to the leak's Play.
//
//   * Per-component 3D listener setters -- FUNCTIONAL. 5849 split the leak's combined
//     SetListenerParameters into Position/Velocity/Orientation. We keep a persistent DS3DLISTENER on
//     the engine, update the relevant component, and delegate to the (real) SetListenerParameters.
//
//   * Runtime variables -- FUNCTIONAL storage. SetVariable/GetVariable round-trip through a table on
//     the engine. (What is NOT implemented is RPC modulation -- variables driving sound parameters --
//     which needs an RPC/curve engine the leak does not have.)
//
//   * SetParameterControl -- the RPC/parameter-control engine is entirely new in 5849 and absent
//     from the leak; this remains a no-op stub (returns S_OK) so titles link and boot.

#include "xacti.h"

using namespace XACT;

// ---- CEngine 5849 methods --------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CEngine::SetListenerPosition(FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    m_ds3dListener.vPosition.x = x;
    m_ds3dListener.vPosition.y = y;
    m_ds3dListener.vPosition.z = z;
    return SetListenerParameters(&m_ds3dListener, NULL, dwApply);
}

HRESULT STDMETHODCALLTYPE CEngine::SetListenerVelocity(FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    m_ds3dListener.vVelocity.x = x;
    m_ds3dListener.vVelocity.y = y;
    m_ds3dListener.vVelocity.z = z;
    return SetListenerParameters(&m_ds3dListener, NULL, dwApply);
}

HRESULT STDMETHODCALLTYPE CEngine::SetListenerOrientation(FLOAT xFront, FLOAT yFront, FLOAT zFront,
                                                          FLOAT xTop, FLOAT yTop, FLOAT zTop, DWORD dwApply)
{
    m_ds3dListener.vOrientFront.x = xFront;
    m_ds3dListener.vOrientFront.y = yFront;
    m_ds3dListener.vOrientFront.z = zFront;
    m_ds3dListener.vOrientTop.x = xTop;
    m_ds3dListener.vOrientTop.y = yTop;
    m_ds3dListener.vOrientTop.z = zTop;
    return SetListenerParameters(&m_ds3dListener, NULL, dwApply);
}

HRESULT STDMETHODCALLTYPE CEngine::SetVariable(DWORD dwVariable, WORD wValue)
{
    if (dwVariable >= (sizeof(m_aVariables) / sizeof(m_aVariables[0])))
        return E_INVALIDARG;
    m_aVariables[dwVariable] = wValue;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CEngine::GetVariable(DWORD dwVariable, PWORD pwValue)
{
    if (!pwValue)
        return E_POINTER;
    if (dwVariable >= (sizeof(m_aVariables) / sizeof(m_aVariables[0])))
        return E_INVALIDARG;
    *pwValue = m_aVariables[dwVariable];
    return S_OK;
}

// ---- exported C entry points -----------------------------------------------------------------

STDAPI IXACTSoundBank_PrepareEx(PXACTSOUNDBANK pBank, PCXACT_PREPARE_SOUNDCUE pPrepareData, PXACTSOUNDCUE *ppCue)
{
    // No prepared-but-stopped state in the leak runtime: create+play the cue instead.
    return ((CSoundBank *)pBank)->Play(pPrepareData->dwCueIndex, pPrepareData->pSoundSource,
                                       pPrepareData->dwFlags, ppCue);
}

STDAPI IXACTEngine_SetVariable(PXACTENGINE pEngine, DWORD dwVariable, WORD wValue, DWORD /*dwApply*/)
{
    return ((CEngine *)pEngine)->SetVariable(dwVariable, wValue);
}

STDAPI IXACTEngine_GetVariable(PXACTENGINE pEngine, DWORD dwVariable, PWORD pwValue)
{
    return ((CEngine *)pEngine)->GetVariable(dwVariable, pwValue);
}

STDAPI IXACTEngine_SetParameterControl(PXACTENGINE /*pEngine*/, PCXACT_PARAMETER_CONTROL_DESC /*pParams*/)
{
    // The RPC/parameter-control engine is new in 5849 and absent from the leak; accept and ignore.
    return S_OK;
}

STDAPI IXACTEngine_SetListenerPosition(PXACTENGINE pEngine, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    return ((CEngine *)pEngine)->SetListenerPosition(x, y, z, dwApply);
}

STDAPI IXACTEngine_SetListenerVelocity(PXACTENGINE pEngine, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
{
    return ((CEngine *)pEngine)->SetListenerVelocity(x, y, z, dwApply);
}

STDAPI IXACTEngine_SetListenerOrientation(PXACTENGINE pEngine, FLOAT xFront, FLOAT yFront, FLOAT zFront,
                                          FLOAT xTop, FLOAT yTop, FLOAT zTop, DWORD dwApply)
{
    return ((CEngine *)pEngine)->SetListenerOrientation(xFront, yFront, zFront, xTop, yTop, zTop, dwApply);
}

// ---- 5849 DSP image download (renamed LoadDspImage + returns the image desc) -----------------

HRESULT STDMETHODCALLTYPE CEngine::DownloadEffectsImage(PVOID pvData, DWORD dwSize,
                                                        LPCDSEFFECTIMAGELOC pEffectLoc,
                                                        LPDSEFFECTIMAGEDESC *ppImageDesc)
{
    HRESULT hr = LoadDspImage(pvData, dwSize, pEffectLoc);
    if (ppImageDesc) {
        *ppImageDesc = SUCCEEDED(hr) ? m_pDspImageDesc : NULL;
    }
    return hr;
}

STDAPI IXACTEngine_DownloadEffectsImage(PXACTENGINE pEngine, PVOID pvData, DWORD dwSize,
                                        LPCDSEFFECTIMAGELOC pEffectLoc, LPDSEFFECTIMAGEDESC *ppImageDesc)
{
    return ((CEngine *)pEngine)->DownloadEffectsImage(pvData, dwSize, pEffectLoc, ppImageDesc);
}

// ---- 5849 sound-source cue stop --------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CSoundSource::StopSoundCues()
{
    // 5849 stops the cues playing through this source; the leak's source owns a single
    // hardware voice, so stopping that voice stops whatever those cues are rendering.
    return Stop();
}

STDAPI IXACTSoundSource_StopSoundCues(PXACTSOUNDSOURCE pSoundSource)
{
    return ((CSoundSource *)pSoundSource)->StopSoundCues();
}
