/**************************************************************************
 *
 *  Copyright (C) 2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       xact.h
 *  Content:    X-Box Audio Content Tool Runtime Engine.
//@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By       Reason
 *   ====       ==       ======
 *  01/17/2002  georgioc Created.
//@@END_MSINTERNAL
 *
 **************************************************************************/

#ifndef __XACT_ENGINE_INCLUDED__
#define __XACT_ENGINE_INCLUDED__

#pragma warning(disable:4201)

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(p)
#endif // UNREFERENCED_PARAMETER

//
// Forward declarations
//

typedef struct IXACTEngine IXACTEngine;
typedef IXACTEngine *LPXACTENGINE;
typedef IXACTEngine *PXACTENGINE;

typedef struct IXACTSoundBank IXACTSoundBank;
typedef IXACTSoundBank *LPXACTSOUNDBANK;
typedef IXACTSoundBank *PXACTSOUNDBANK;

typedef struct IXACTSoundSource IXACTSoundSource;
typedef IXACTSoundSource *LPXACTSOUNDSOURCE;
typedef IXACTSoundSource *PXACTSOUNDSOURCE;

typedef struct IXACTSoundCue IXACTSoundCue;
typedef IXACTSoundCue *LPXACTSOUNDCUE;
typedef IXACTSoundCue *PXACTSOUNDCUE;

typedef struct IXACTWaveBank IXACTWaveBank;
typedef IXACTWaveBank *LPXACTWAVEBANK;
typedef IXACTWaveBank *PXACTWAVEBANK;

//@@BEGIN_MSINTERNAL
typedef struct XACT_TRACK_EVENT XACT_TRACK_EVENT;
typedef XACT_TRACK_EVENT *PXACT_TRACK_EVENT;
typedef XACT_TRACK_EVENT *LPXACT_TRACK_EVENT;
//@@END_MSINTERNAL
//
// Structures and types
//

#define XACT_SIZEOF_MARKER_DATA		8

//
// Notifications
//

typedef enum _XACT_NOTIFICATION_TYPE {

    eXACTNotification_Start = 0,
    eXACTNotification_Stop,
    eXACTNotification_Marker,
	eXACTNotification_Max

};

#define XACT_MASK_NOTIFICATION_TYPE		0x0000FFFF

// RXDK 5849 uplift: the notification type/flags moved from one DWORD dwType (type in the
// low word, flags in the high word) to a WORD wType + a separate WORD wFlags. The type
// sentinel is therefore a WORD, and PERSIST relocated from 0x00010000 to wFlags bit 0x8000.
#define XACT_NOTIFICATION_TYPE_UNUSED	0xFFFF

//
// flags used when registering notifications (5849: the wFlags WORD)
//

#define XACT_FLAG_NOTIFICATION_USE_WAVEBANK          0x0001
#define XACT_FLAG_NOTIFICATION_USE_SOUNDCUE_INDEX    0x0002
#define XACT_FLAG_NOTIFICATION_USE_SOUNDCUE_INSTANCE 0x0004
#define XACT_FLAG_NOTIFICATION_SOUNDCUE_DESTROYED    0x0008
#define XACT_FLAG_NOTIFICATION_PERSIST	             0x8000

#define XACT_MASK_NOTIFICATION_FLAGS	(XACT_FLAG_NOTIFICATION_USE_SOUNDCUE_INSTANCE | XACT_FLAG_NOTIFICATION_USE_WAVEBANK | XACT_FLAG_NOTIFICATION_USE_SOUNDCUE_INDEX)


typedef struct _XACT_NOTIFICATION_START {

    DWORD dwFlags;

} XACT_NOTIFICATION_START, *PXACT_NOTIFICATION_START, *LPXACT_NOTIFICATION_START;
 
typedef struct _XACT_NOTIFICATION_STOP {

    DWORD dwFlags;

} XACT_NOTIFICATION_STOP, *PXACT_NOTIFICATION_STOP, *LPXACT_NOTIFICATION_STOP;
 
typedef struct _XACT_NOTIFICATION_MARKER {

    DWORD   dwData;     // 5849: the delivered marker is the authored Value (a single DWORD)

} XACT_NOTIFICATION_MARKER, *PXACT_NOTIFICATION_MARKER, *LPXACT_NOTIFICATION_MARKER;

union XACT_NOTIFICATION_UNION {

    XACT_NOTIFICATION_START Start;
    XACT_NOTIFICATION_STOP Stop;
    XACT_NOTIFICATION_MARKER Marker;

}; 

typedef struct _XACT_NOTIFICATION_DESCRIPTION{

    WORD             wType;
    WORD             wFlags;

    union {
        PXACTSOUNDBANK   pSoundBank;
        PXACTWAVEBANK    pWaveBank;
    } u;

    DWORD            dwSoundCueIndex;
    PXACTSOUNDCUE    pSoundCue;

    PVOID            pvContext;
    HANDLE           hEvent;

} XACT_NOTIFICATION_DESCRIPTION, *PXACT_NOTIFICATION_DESCRIPTION, *LPXACT_NOTIFICATION_DESCRIPTION;

typedef struct _XACT_NOTIFICATION{
    

    XACT_NOTIFICATION_DESCRIPTION	Header;
    XACT_NOTIFICATION_UNION			Data;
    REFERENCE_TIME					rtTimeStamp;

} XACT_NOTIFICATION, *PXACT_NOTIFICATION, *LPXACT_NOTIFICATION;        
 
typedef struct _XACT_RUNTIME_PARAMETERS {
    DWORD dwMax2DHwVoices;
    DWORD dwMax3DHwVoices;
    DWORD dwMaxConcurrentStreams;
    DWORD dwMaxNotifications;               // 5849: replaced pvHeap (engine never read it)
    DWORD dwInteractiveAudioLookaheadTime;  // 5849: replaced dwHeapSize
} XACT_RUNTIME_PARAMETERS, *PXACT_RUNTIME_PARAMETERS, *LPXACT_RUNTIME_PARAMETERS;

// 5849: runtime sound source properties (IXACTSoundSource_GetProperties).
// Mirrored from the public xact.h for the same shadowing reason as
// XACT_FLAG_SOUNDSOURCE_STATUS_ACTIVE below.
typedef struct _XACT_SOUNDSOURCE_PROPERTIES {
    DWORD           dwHighestCuePriority;
    DSVOICEPROPS    HwVoiceProperties;
} XACT_SOUNDSOURCE_PROPERTIES, *PXACT_SOUNDSOURCE_PROPERTIES;

// 5849: IXACTSoundBank_SelectVariation data (mirrored from the public xact.h).

#define XACT_FLAG_SELECT_VARIATION_SOUND_INDEX  0x00000001
#define XACT_FLAG_SELECT_VARIATION_SOUND_VALUE  0x00000002
#define XACT_FLAG_SELECT_VARIATION_SOUND_FLAGS  (XACT_FLAG_SELECT_VARIATION_SOUND_INDEX | XACT_FLAG_SELECT_VARIATION_SOUND_VALUE)
#define XACT_FLAG_SELECT_VARIATION_WAVE_INDEX   0x00000004
#define XACT_FLAG_SELECT_VARIATION_WAVE_VALUE   0x00000008
#define XACT_FLAG_SELECT_VARIATION_WAVE_FLAGS   (XACT_FLAG_SELECT_VARIATION_WAVE_INDEX | XACT_FLAG_SELECT_VARIATION_WAVE_VALUE)
#define XACT_FLAG_SELECT_VARIATION_FLAGS        (XACT_FLAG_SELECT_VARIATION_SOUND_FLAGS | XACT_FLAG_SELECT_VARIATION_WAVE_FLAGS)

typedef struct _XACT_SOUNDBANK_SELECT_VARIATION
{
    DWORD   dwFlags;

    union
    {
        DWORD   dwIndex;
        FLOAT   flValue;
    } Sound;

    union
    {
        DWORD   dwIndex;
        FLOAT   flValue;
    } Wave;
} XACT_SOUNDBANK_SELECT_VARIATION, *PXACT_SOUNDBANK_SELECT_VARIATION;

typedef const XACT_SOUNDBANK_SELECT_VARIATION *PCXACT_SOUNDBANK_SELECT_VARIATION;

// 5849: IXACTSoundBank_GetSoundCueProperties (mirrored from the public xact.h).

#define XACT_WAVE_INDEX_UNUSED  0xFFFF

#define XACT_FLAG_SOUNDCUE_PROPERTIES_3D    0x00000001
#define XACT_FLAG_SOUNDCUE_PROPERTIES_FLAGS (XACT_FLAG_SOUNDCUE_PROPERTIES_3D)

typedef struct _XACT_SOUNDCUE_PROPERTIES
{
    DWORD   dwFlags;            // Flags
    DWORD   dwPriority;         // Priority
    LONG    lVolume;            // Sound volume (dB * 100)
    LONG    lPitch;             // Pitch (semitone * 4096 / 12)
    DWORD   dwLayer;            // Layer
    DWORD   dwCategory;         // Category (0-based, or 255 if sound has no category)
    LONG    lParametricEQGain;  // Parametric EQ gain [-8192, 32767]
    DWORD   dwParametricEQQ;    // Parametric EQ Q [0, 4]
    DWORD   dwParametricEQFc;   // Parametric EQ frequency
    DWORD   dwLoopCount;        // Loop count (highest loop count of all tracks)
    DWORD   dwTrackCount;       // Number of tracks
    DWORD   dwSoundIndex;       // Sound index
    DWORD   dwWaveIndex;        // Wave index
    DWORD   dwLength;           // Length in ms

    // 3D properties

    LONG    lI3DL2Volume;       // I3DL2 volume send (dB * 100)
    LONG    lLFEVolume;         // LFE volume send (dB * 100)
    DWORD   dwInsideConeAngle;  // Buffer inside cone angle
    DWORD   dwOutsideConeAngle; // Buffer outside cone angle
    LONG    lConeOutsideVolume; // Volume outside the cone
    DWORD   dwMode;             // 3D processing mode
    FLOAT   flMinDistance;      // Minimum distance value
    FLOAT   flMaxDistance;      // Maximum distance value
    FLOAT   flDistanceFactor;   // Distance factor
    FLOAT   flRolloffFactor;    // Rolloff factor
    FLOAT   flDopplerFactor;    // Doppler factor
} XACT_SOUNDCUE_PROPERTIES, *PXACT_SOUNDCUE_PROPERTIES;

// 5849: IXACTEngine_GetRealtimeData (mirrored from the public xact.h).

typedef struct _XACT_REALTIME_AUDIO_DATA
{
    DSOUTPUTLEVELS  OutputLevels;
    DSCAPS          DSoundCaps;
    DWORD           dwXactMemoryUsage;
    BYTE            bXactAvailable2DBuffers;
    BYTE            bXactAvailable2DStreams;
    BYTE            bXactAvailable3DBuffers;
    BYTE            bReserved;
} XACT_REALTIME_AUDIO_DATA, *PXACT_REALTIME_AUDIO_DATA;

//
// constants
//

#define XACT_FLAG_SOUNDSOURCE_2D            0x00000001
#define XACT_FLAG_SOUNDSOURCE_3D            0x00000002
#define XACT_MASK_SOUNDSOURCE_FLAGS         (XACT_FLAG_SOUNDSOURCE_3D | XACT_FLAG_SOUNDSOURCE_2D)

// 5849: IXACTSoundSource_GetStatus's only defined bit. The leak-era inc/ headers
// shadow shared/include, so a constant added only to the public xact.h is
// invisible to the engine that has to set it.
#define XACT_FLAG_SOUNDSOURCE_STATUS_ACTIVE 0x00000001

#define XACT_FLAG_SOUNDCUE_AUTORELEASE			0x00000001
#define XACT_FLAG_SOUNDCUE_SYNCHRONOUS          0x10000000

#define XACT_SOUNDCUE_INDEX_UNUSED				0xFFFFFFFF
#define XACT_TRACK_INDEX_UNUSED	    			0xFFFFFFFF

//
// API definitions
//

//
// IXACTEngine
//

STDAPI XACTEngineCreate(PXACT_RUNTIME_PARAMETERS pParams, PXACTENGINE *ppEngine);  // 5849: args reversed
STDAPI_(void) XACTEngineDoWork(void);

STDAPI_(ULONG) IXACTEngine_AddRef(PXACTENGINE pEngine);
STDAPI_(ULONG) IXACTEngine_Release(PXACTENGINE pEngine);
STDAPI IXACTEngine_LoadDspImage(PXACTENGINE pEngine, PVOID pvData, DWORD dwSize, LPCDSEFFECTIMAGELOC pEffectLoc);
STDAPI IXACTEngine_CreateSoundSource(PXACTENGINE pEngine, DWORD dwFlags, PXACTSOUNDSOURCE *ppSoundSource);
STDAPI IXACTEngine_CreateSoundBank(PXACTENGINE pEngine, PVOID pvData, DWORD dwSize, PXACTSOUNDBANK *ppSoundBank);
STDAPI IXACTEngine_RegisterWaveBank(PXACTENGINE pEngine, PVOID pvData, DWORD dwSize, PXACTWAVEBANK * ppWaveBank);

// RXDK 5849 uplift: 5849 replaced the leak's (pvStreamingBuffer,dwSize,hFile,dwOffset) form with a
// params block -- the runtime owns the buffer, sized from the packet timing.
typedef struct _XACT_WAVEBANK_STREAMING_PARAMETERS
{
    HANDLE  hFile;
    DWORD   dwOffset;
    DWORD   dwPacketSizeInMilliSecs;
    DWORD   dwPrimePacketSizeInMilliSecs;
} XACT_WAVEBANK_STREAMING_PARAMETERS, *PXACT_WAVEBANK_STREAMING_PARAMETERS;
typedef const XACT_WAVEBANK_STREAMING_PARAMETERS *PCXACT_WAVEBANK_STREAMING_PARAMETERS;

STDAPI IXACTEngine_RegisterStreamedWaveBank(PXACTENGINE pEngine, PCXACT_WAVEBANK_STREAMING_PARAMETERS pParams, PXACTWAVEBANK *ppWaveBank);

// RXDK 5849 uplift: the interactive-audio runtime-variable + parameter-control subsystems are new
// in 5849 -- the Jan-2002 leak has no such runtime. These exports exist so 5849 titles link and
// boot; they are no-op stubs (return S_OK) and do NOT modulate audio. See engine/uplift5849.cpp.
typedef const void *PCXACT_PARAMETER_CONTROL_DESC;
STDAPI IXACTEngine_SetVariable(PXACTENGINE pEngine, DWORD dwVariable, WORD wValue, DWORD dwApply);
STDAPI IXACTEngine_GetVariable(PXACTENGINE pEngine, DWORD dwVariable, PWORD pwValue);
STDAPI IXACTEngine_SetParameterControl(PXACTENGINE pEngine, PCXACT_PARAMETER_CONTROL_DESC pParams);
STDAPI IXACTEngine_UnRegisterWaveBank(PXACTENGINE pEngine, PXACTWAVEBANK pWaveBank);
STDAPI IXACTEngine_SetMasterVolume(PXACTENGINE pEngine, WORD wCategory, LONG lVolume);  // 5849: +wCategory
STDAPI IXACTEngine_SetListenerParameters(PXACTENGINE pEngine, LPCDS3DLISTENER pcDs3dListener, LPCDSI3DL2LISTENER pds3dl, DWORD dwApply);
// RXDK 5849 uplift: 5849 split the combined SetListenerParameters into per-component setters.
// The leak has no per-component listener path, so these are no-op stubs (see engine/uplift5849.cpp).
STDAPI IXACTEngine_SetListenerPosition(PXACTENGINE pEngine, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply);
STDAPI IXACTEngine_SetListenerVelocity(PXACTENGINE pEngine, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply);
STDAPI IXACTEngine_SetListenerOrientation(PXACTENGINE pEngine, FLOAT xFront, FLOAT yFront, FLOAT zFront, FLOAT xTop, FLOAT yTop, FLOAT zTop, DWORD dwApply);
STDAPI IXACTEngine_GlobalPause(PXACTENGINE pEngine, WORD wCategory, BOOL bPause);
STDAPI IXACTEngine_RegisterNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc);
STDAPI IXACTEngine_UnRegisterNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc);
STDAPI IXACTEngine_GetNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc, PXACT_NOTIFICATION pNotification);
STDAPI IXACTEngine_FlushNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc);
STDAPI IXACTEngine_CommitDeferredSettings(PXACTENGINE pEngine);
//@@BEGIN_MSINTERNAL
STDAPI IXACTEngine_ScheduleEvent(PXACTENGINE pEngine, XACT_TRACK_EVENT *pEventDesc, PXACTSOUNDCUE pSoundCue, DWORD dwTrackIndex);
//@@END_MSINTERNAL

#if defined(__cplusplus) && !defined(CINTERFACE)

struct IXACTEngine
{

    __inline ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return IXACTEngine_AddRef(this);
    }

    __inline ULONG STDMETHODCALLTYPE Release(void)
    {
        return IXACTEngine_Release(this);
    }

    __inline HRESULT STDMETHODCALLTYPE LoadDspImage(PVOID pvData, DWORD dwSize, LPCDSEFFECTIMAGELOC pEffectLoc)
    {
        return IXACTEngine_LoadDspImage(this, pvData, dwSize, pEffectLoc);
    }

    __inline HRESULT STDMETHODCALLTYPE CreateSoundSource(DWORD dwFlags,PXACTSOUNDSOURCE *ppSoundSource)
    {
        return IXACTEngine_CreateSoundSource(this, dwFlags, ppSoundSource);
    }

    __inline HRESULT STDMETHODCALLTYPE CreateSoundBank(PVOID pvData, DWORD dwSize, PXACTSOUNDBANK *ppSoundBank)
    {
        return IXACTEngine_CreateSoundBank(this, pvData, dwSize, ppSoundBank);
    }

    __inline HRESULT STDMETHODCALLTYPE RegisterWaveBank(PVOID pvData, DWORD dwSize, PXACTWAVEBANK *ppWaveBank)
    {
        return IXACTEngine_RegisterWaveBank(this, pvData, dwSize, ppWaveBank);
    }

    __inline HRESULT STDMETHODCALLTYPE RegisterStreamedWaveBank(PCXACT_WAVEBANK_STREAMING_PARAMETERS pParams, PXACTWAVEBANK *ppWaveBank)
    {
        return IXACTEngine_RegisterStreamedWaveBank(this, pParams, ppWaveBank);
    }

    __inline HRESULT STDMETHODCALLTYPE UnRegisterWaveBank(PXACTWAVEBANK pWaveBank)
    {
        return IXACTEngine_UnRegisterWaveBank(this, pWaveBank);
    }

    __inline HRESULT STDMETHODCALLTYPE SetMasterVolume(WORD wCategory, LONG lVolume)
    {
        return IXACTEngine_SetMasterVolume(this, wCategory, lVolume);
    }

    __inline HRESULT STDMETHODCALLTYPE SetListenerParameters(LPCDS3DLISTENER pcDs3dListener, LPCDSI3DL2LISTENER pds3dl, DWORD dwApply)
	{
	    return IXACTEngine_SetListenerParameters(this, pcDs3dListener, pds3dl, dwApply);
	}

    __inline HRESULT STDMETHODCALLTYPE GlobalPause(WORD wCategory, BOOL bPause)
    {
        return IXACTEngine_GlobalPause(this, wCategory, bPause);
    }

    __inline HRESULT STDMETHODCALLTYPE RegisterNotification(PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc)
    {
        return IXACTEngine_RegisterNotification(this, pNotificationDesc);
    }

    __inline HRESULT STDMETHODCALLTYPE UnRegisterNotification(PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc)
    {
        return IXACTEngine_UnRegisterNotification(this, pNotificationDesc);
    }

    __inline HRESULT STDMETHODCALLTYPE GetNotification(PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc, PXACT_NOTIFICATION pNotification)
    {
        return IXACTEngine_GetNotification(this, pNotificationDesc, pNotification);
    }

    __inline HRESULT STDMETHODCALLTYPE FlushNotification(PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc)
    {
        return IXACTEngine_FlushNotification(this, pNotificationDesc);
    }

    __inline HRESULT STDMETHODCALLTYPE CommitDeferredSettings(void)
    {
        return IXACTEngine_CommitDeferredSettings(this);
    }

//@@BEGIN_MSINTERNAL
    __inline HRESULT STDMETHODCALLTYPE ScheduleEvent(XACT_TRACK_EVENT *pEventDesc, PXACTSOUNDCUE pSoundCue, DWORD dwTrackIndex)
	{
        return IXACTEngine_ScheduleEvent(this, pEventDesc, pSoundCue, dwTrackIndex);
	}
//@@END_MSINTERNAL

};

#endif // defined(__cplusplus) && !defined(CINTERFACE)

//
// IXACTSoundSource
//

STDAPI_(ULONG) IXACTSoundSource_AddRef(PXACTSOUNDSOURCE pSoundSource);
STDAPI_(ULONG) IXACTSoundSource_Release(PXACTSOUNDSOURCE pSoundSource);
STDAPI IXACTSoundSource_SetPosition(PXACTSOUNDSOURCE pSoundSource, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply);
STDAPI IXACTSoundSource_SetAllParameters(PXACTSOUNDSOURCE pSoundSource, LPCDS3DBUFFER pcDs3dBuffer, DWORD dwApply);
STDAPI IXACTSoundSource_SetConeOrientation(PXACTSOUNDSOURCE pSoundSource,FLOAT x, FLOAT y, FLOAT z, DWORD dwApply);
STDAPI IXACTSoundSource_SetI3DL2Source(PXACTSOUNDSOURCE pSoundSource,LPCDSI3DL2BUFFER pds3db, DWORD dwApply);
STDAPI IXACTSoundSource_SetVelocity(PXACTSOUNDSOURCE pSoundSource,FLOAT x, FLOAT y, FLOAT z, DWORD dwApply);
STDAPI IXACTSoundSource_SetMixBins(PXACTSOUNDSOURCE pSoundSource, LPCDSMIXBINS pMixBins);
STDAPI IXACTSoundSource_SetMixBinVolumes(PXACTSOUNDSOURCE pSoundSource, LPCDSMIXBINS pMixBins);

#if defined(__cplusplus) && !defined(CINTERFACE)

struct IXACTSoundSource
{

    __inline ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return IXACTSoundSource_AddRef(this);
    }

    __inline ULONG STDMETHODCALLTYPE Release(void)
    {
        return IXACTSoundSource_Release(this);
    }

    __inline HRESULT STDMETHODCALLTYPE SetPosition( FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
    {
        return IXACTSoundSource_SetPosition(this, x, y, z, dwApply);
    }

    __inline HRESULT STDMETHODCALLTYPE SetAllParameters(LPCDS3DBUFFER pcDs3dBuffer, DWORD dwApply)
    {
        return IXACTSoundSource_SetAllParameters(this, pcDs3dBuffer, dwApply);
    }

    __inline HRESULT STDMETHODCALLTYPE SetConeOrientation(FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
    {
        return IXACTSoundSource_SetConeOrientation(this, x, y, z, dwApply);
    }

    __inline HRESULT STDMETHODCALLTYPE SetI3DL2Source(LPCDSI3DL2BUFFER pds3db, DWORD dwApply)
    {
        return IXACTSoundSource_SetI3DL2Source(this, pds3db, dwApply);
    }

    __inline HRESULT STDMETHODCALLTYPE SetVelocity(FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
    {
        return IXACTSoundSource_SetVelocity(this, x,  y,  z, dwApply);
    }

    __inline HRESULT STDMETHODCALLTYPE SetMixBins(LPCDSMIXBINS pMixBins)
    {
        return IXACTSoundSource_SetMixBins(this, pMixBins);
    }

    __inline HRESULT STDMETHODCALLTYPE SetMixBinVolumes(LPCDSMIXBINS pMixBins)
    {
        return IXACTSoundSource_SetMixBinVolumes(this, pMixBins);
    }

};

#endif // defined(__cplusplus) && !defined(CINTERFACE)

//
// IXACTSoundBank
//

STDAPI_(ULONG) IXACTSoundBank_AddRef(PXACTSOUNDBANK pBank);
STDAPI_(ULONG) IXACTSoundBank_Release(PXACTSOUNDBANK pBank);
STDAPI IXACTSoundBank_GetSoundCueIndexFromFriendlyName(PXACTSOUNDBANK pBank, LPCSTR lpFriendlyName, PDWORD pdwSoundCueIndex);
STDAPI IXACTSoundBank_Play(PXACTSOUNDBANK pBank, DWORD dwSoundCueIndex, PXACTSOUNDSOURCE pSoundSource, DWORD dwFlags, PXACTSOUNDCUE *ppSoundCue);
STDAPI IXACTSoundBank_Stop(PXACTSOUNDBANK pBank, DWORD dwSoundCueIndex, DWORD dwFlags, PXACTSOUNDCUE pSoundCue);
STDAPI IXACTSoundBank_SetSliderValue(PXACTSOUNDBANK pBank, DWORD dwSoundCueIndex, DWORD dwSliderIndex, PVOID pvValue);

// RXDK 5849 uplift: PlayEx entry point + its parameter block (matches the 5849 public
// xact.h). pParameterControls is carried for layout compatibility but not honored here.
typedef struct _XACT_PREPARE_SOUNDCUE
{
    DWORD               dwFlags;
    DWORD               dwCueIndex;
    PXACTSOUNDSOURCE    pSoundSource;
    const void         *pParameterControls;
} XACT_PREPARE_SOUNDCUE, *PXACT_PREPARE_SOUNDCUE;
typedef const XACT_PREPARE_SOUNDCUE *PCXACT_PREPARE_SOUNDCUE;

STDAPI IXACTSoundBank_PlayEx(PXACTSOUNDBANK pBank, PCXACT_PREPARE_SOUNDCUE pPrepareData, PXACTSOUNDCUE *ppCue);
STDAPI IXACTSoundBank_PrepareEx(PXACTSOUNDBANK pBank, PCXACT_PREPARE_SOUNDCUE pPrepareData, PXACTSOUNDCUE *ppCue);

#if defined(__cplusplus) && !defined(CINTERFACE)

struct IXACTSoundBank
{

    __inline ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return IXACTSoundBank_AddRef(this);
    }

    __inline ULONG STDMETHODCALLTYPE Release(void)
    {
        return IXACTSoundBank_Release(this);
    }

    __inline HRESULT STDMETHODCALLTYPE GetSoundCueIndexFromFriendlyName(LPCSTR lpFriendlyName, PDWORD pdwSoundCueIndex)
    {
        return IXACTSoundBank_GetSoundCueIndexFromFriendlyName(this, lpFriendlyName, pdwSoundCueIndex);
    }

    __inline HRESULT STDMETHODCALLTYPE Play( DWORD dwSoundCueIndex, PXACTSOUNDSOURCE pSoundSource, DWORD dwFlags, PXACTSOUNDCUE *ppSoundCue)
    {
        return IXACTSoundBank_Play(this, dwSoundCueIndex, pSoundSource, dwFlags, ppSoundCue);
    }

    __inline HRESULT STDMETHODCALLTYPE Stop( DWORD dwSoundCueIndex, DWORD dwFlags, PXACTSOUNDCUE pSoundCue)
    {
        return IXACTSoundBank_Stop(this, dwSoundCueIndex, dwFlags, pSoundCue);
    }

    __inline HRESULT STDMETHODCALLTYPE SetSliderValue(DWORD dwSoundCueIndex, DWORD dwSliderIndex, PVOID pvValue)
    {
        return IXACTSoundBank_SetSliderValue(this, dwSoundCueIndex, dwSliderIndex, pvValue);
    }

};

#endif // defined(__cplusplus) && !defined(CINTERFACE)


//@@BEGIN_MSINTERNAL

#define XACT_SOUNDBANK_HEADER_FRIENDLYNAME_LENGTH 16
#define XACT_SOUNDBANK_CUE_FRIENDLYNAME_LENGTH 16
#define XACT_SOUNDBANK_WAVEBANK_FRIENDLYNAME_LENGTH 16

#define XACT_SOUNDBANK_HEADER_SIGNATURE        'KBDS'

// A sound with no category. The public xact.h spells the cue-properties form as
// XACT_CATEGORY_INDEX_UNUSED = 0xFF; the bank field is a WORD, so widen it here
// rather than truncating and colliding with a real category 255.
#define XACT_SOUNDBANK_CATEGORY_UNUSED         0xFFFF
// RXDK: bumped 1 -> 2 when wCategory was added to XACT_SOUNDBANK_SOUND_ENTRY.
// The entry stride changed, so a v1 bank cannot be read with the v2 struct --
// but CSoundBank::IsValidHeader rejects a mismatched version outright, so a
// stale .xsb produces a clean error instead of misread audio. Rebuild banks.
#define XACT_SOUNDBANK_HEADER_VERSION          2

typedef struct _XACT_SOUNDBANK_FILE_HEADER{

    DWORD    dwSignature;
    DWORD    dwVersion;
    DWORD    dwFlags;
    DWORD    dwSoundEntryCount;                 // Number of entries in the bank
    DWORD    dwCueEntryCount;                   // Number of cues in the bank;
    CHAR     szFriendlyName[XACT_SOUNDBANK_HEADER_FRIENDLYNAME_LENGTH];   // friendly name

} XACT_SOUNDBANK_FILE_HEADER, *PXACT_SOUNDBANK_FILE_HEADER, *LPXACT_SOUNDBANK_FILE_HEADER; 


//
// content flags defining CU behavior
//

#define XACT_FLAG_CUE_ENTRY_QUEUE     		0x00000001
#define XACT_FLAG_CUE_ENTRY_CROSSFADE 		0x00000002

//
// table of N cue entries follows the header
//

typedef struct _XACT_SOUNDBANK_CUE_ENTRY{

    DWORD    dwFlags;
    DWORD    dwSoundIndex;
    CHAR     szFriendlyName[XACT_SOUNDBANK_CUE_FRIENDLYNAME_LENGTH];

} XACT_SOUNDBANK_CUE_ENTRY, *PXACT_SOUNDBANK_CUE_ENTRY, *LPXACT_SOUNDBANK_CUE_ENTRY; 


#define XACT_FLAG_SOUND_3D              0x00000001
#define XACT_FLAG_SOUND_FXMULTIPASS     0x00000002

#define XACTMIXBINVOLUMEPAIR DSMIXBINVOLUMEPAIR

//
// table of N sound entries follows the cue table
//

typedef struct _XACT_SOUNDBANK_SOUND_ENTRY{

    DWORD					dwFlags;
    DWORD					dw3DParametersOffset;
    DWORD					dwTrackTableOffset;
    DWORD					dwWaveBankTableOffset;    
	WORD					wPriority;
	WORD					wLayer;
    WORD					wGroupNumber;
    WORD					wTrackCount;
    WORD					wWaveBankCount;
	WORD					wSliderCount;

    //
    // RXDK 5849 uplift: the sound's mix category, as an index into the
    // categories the .xap declares, or XACT_SOUNDBANK_CATEGORY_UNUSED.
    //
    // 5849 exposes categories through the wCategory selector on GlobalPause
    // and SetMasterVolume, and reports one in XACT_SOUNDCUE_PROPERTIES. The
    // leak's format had nowhere to put it -- wGroupNumber above is variation
    // selection, not a mix category -- so there was nothing to select on.
    //
    WORD					wCategory;

} XACT_SOUNDBANK_SOUND_ENTRY, *PXACT_SOUNDBANK_SOUND_ENTRY, *LPXACT_SOUNDBANK_SOUND_ENTRY; 

//
// 3d parameters data structure that can optionally be associated with a sound
//

typedef struct _XACT_SOUNDBANK_SOUND_3D_PARAMETERS {

    XACTMIXBINVOLUMEPAIR	aVolumePair;	// volume for 8th mixbin on a 3d destination
    DWORD    dwInsideConeAngle;      // Buffer inside cone angle
    DWORD    dwOutsideConeAngle;     // Buffer outside cone angle
    LONG     lConeOutsideVolume;     // Volume outside the cone
    FLOAT    flMinDistance;          // Minimum distance value
    FLOAT    flMaxDistance;          // Maximum distance value
    DWORD    dwMode;                 // 3D processing mode
    FLOAT    flDistanceFactor;       // Distance factor
    FLOAT    flRolloffFactor;        // Rolloff factor
    FLOAT    flDopplerFactor;        // Doppler factor
    DWORD    dwDataEntryCount;       // number of custom rollof data points
    
    //
    // array of FLOATs immediately following if dwTableEntryCount != 0
    //

} XACT_SOUNDBANK_SOUND_3D_PARAMETERS, *PXACT_SOUNDBANK_SOUND_3D_PARAMETERS;


//
// wave banks are associated with a sound through a table. This is because the same wave bank
// can be re-used by multiple sounds so we need something like a handle table to abstract in-between
//
// the table of wavebank offsets follows the sound entry table
//

typedef struct _XACT_SOUNDBANK_WAVEBANK_TABLE_ENTRY {

    CHAR  szFriendlyName[XACT_SOUNDBANK_WAVEBANK_FRIENDLYNAME_LENGTH];
    DWORD dwDataOffset;

} XACT_SOUNDBANK_WAVEBANK_TABLE_ENTRY, *PXACT_SOUNDBANK_WAVEBANK_TABLE_ENTRY, *LPXACT_SOUNDBANK_WAVEBANK_TABLE_ENTRY;

//
// the track table is the array of track entries and follows the wavebank entry table
//

typedef struct _XACT_SOUNDBANK_TRACK_ENTRY {

    WORD wFlags;
    WORD wEventEntryCount;
    DWORD dwEventDataOffset;

} XACT_SOUNDBANK_TRACK_ENTRY, *PXACT_SOUNDBANK_TRACK_ENTRY, *LPXACT_SOUNDBANK_TRACK_ENTRY;

//
// slider data types
//

//
// the slider table is the array of slider entries and follows the track entry table
//

typedef struct _XACT_SOUNDBANK_SLIDER_ENTRY {
	WORD wNumHwParameters;
	WORD wSoundIndex;
	WORD wTrackIndex;
	WORD wReserved;
	DWORD dwMappingTableOffset;
} XACT_SOUNDBANK_SLIDER_ENTRY, *PXACT_SOUNDBANK_SLIDER_ENTRY, *LPXACT_SOUNDBANK_SLIDER_ENTRY;

//
// after the table of slider entries, there is a list of tables of mapping entries.
// each slider points to a table of mapping entry offsets. This way multiple mappings
// can be re-used by different sliders
//

typedef struct _XACT_SLIDER_MAPPING_TABLE_ENTRY {
	DWORD dwDataOffset;
} XACT_SLIDER_MAPPING_TABLE_ENTRY, *PXACT_SLIDER_MAPPING_TABLE_ENTRY, *LPXACT_SLIDER_MAPPING_TABLE_ENTRY;

//
// a slider mapping entry identifies the hw parameter associated with the slider
// and has a N point table of values (the mapping function). The values are of the native
// format of the hw parameter
//

typedef struct _XACT_SLIDER_MAPPING_ENTRY {
	WORD wParameterId;
	WORD wElementCount;
	DWORD dwData[1];

} XACT_SLIDER_MAPPING_ENTRY, *PXACT_SLIDER_MAPPING_ENTRY, *LPXACT_SLIDER_MAPPING_ENTRY;

//
// each track entry points to an array of variable length event entries. the events for all tracks follow
// the slider table
//

//
// Sequencer events
//

#define XACT_FLAG_EVENT_RUNTIME 	    0x00000001
#define XACT_FLAG_EVENT_USES_FXIN   	0x00000002

typedef struct _XACT_TRACK_EVENT_HEADER {

    WORD	wType;	
    WORD	wSize;
	DWORD	dwFlags;    
	ULONG	lSampleTime;

} XACT_TRACK_EVENT_HEADER, *PXACT_TRACK_EVENT_HEADER, *LPXACT_TRACK_EVENT_HEADER;

//
// Structures and types
//

typedef enum _XACT_TRACK_EVENT_TYPES {

    eXACTEvent_Play = 0,
	eXACTEvent_PlayWithPitchAndVolumeVariation, 
    eXACTEvent_Stop,
	eXACTEvent_PitchAndVolumeVariation,
	eXACTEvent_SetFrequency,
	eXACTEvent_SetVolume,
	eXACTEvent_SetHeadroom,
	eXACTEvent_SetLFO,
	eXACTEvent_SetEG,
	eXACTEvent_SetFilter,
	eXACTEvent_Marker,
	eXACTEvent_LoopStart,
	eXACTEvent_LoopEnd,
	eXACTEvent_SetMixBinVolumes,

	//
	// global events
	//

	eXACTEvent_SetEffectData,
	eXACTEvent_Max

} XACT_TRACK_EVENT_TYPES;

typedef struct _XACT_TRACK_EVENT_MARKER {

	BYTE	bData[XACT_SIZEOF_MARKER_DATA];

} XACT_TRACK_EVENT_MARKER, *PXACT_TRACK_EVENT_MARKER, *LPXACT_TRACK_EVENT_MARKER;

typedef struct _XACT_TRACK_EVENT_SETEFFECTDATA {

    WORD	wEffectIndex;
    WORD	wOffset;    
	WORD	wDataSize;
	WORD    wReserved;
	DWORD   dwData[1];

} XACT_TRACK_EVENT_SETEFFECTDATA, *PXACT_TRACK_EVENT_SETEFFECTDATA, *LPXACT_TRACK_EVENT_SETEFFECTDATA;

typedef struct _XACT_TRACK_EVENT_SETFILTER {

    DSFILTERDESC Desc;
    
} XACT_TRACK_EVENT_SETFILTER, *PXACT_TRACK_EVENT_SETFILTER, *LPXACT_TRACK_EVENT_SETFILTER;


typedef struct _XACT_TRACK_EVENT_SETEG {

    DSENVELOPEDESC Desc;
    
} XACT_TRACK_EVENT_SETEG, *PXACT_TRACK_EVENT_SETEG, *LPXACT_TRACK_EVENT_SETEG;

typedef struct _XACT_TRACK_EVENT_SETLFO {

    DSLFODESC Desc;
    
} XACT_TRACK_EVENT_SETLFO, *PXACT_TRACK_EVENT_SETLFO, *LPXACT_TRACK_EVENT_SETLFO;

typedef struct _XACT_TRACK_EVENT_SETHEADROOM {

    WORD wHeadroom;
    
} XACT_TRACK_EVENT_SETHEADROOM, *PXACT_TRACK_EVENT_SETHEADROOM, *LPXACT_TRACK_EVENT_SETHEADROOM;

typedef struct _XACT_TRACK_EVENT_SETVOLUME {

    SHORT sVolume;
    
} XACT_TRACK_EVENT_SETVOLUME, *PXACT_TRACK_EVENT_SETVOLUME, *LPXACT_TRACK_EVENT_SETVOLUME;

typedef struct _XACT_TRACK_EVENT_SETMIXBINVOLUMES {

	DWORD	dwCount;
    XACTMIXBINVOLUMEPAIR aVolumePairs[8];
    
} XACT_TRACK_EVENT_SETMIXBINVOLUMES, *PXACT_TRACK_EVENT_SETMIXBINVOLUMES, *LPXACT_TRACK_EVENT_SETMIXBINVOLUMES;

typedef struct _XACT_TRACK_EVENT_SETFREQUENCY {

    WORD wFrequency;
    
} XACT_TRACK_EVENT_SETFREQUENCY, *PXACT_TRACK_EVENT_SETFREQUENCY, *LPXACT_TRACK_EVENT_SETFREQUENCY;
 
typedef struct _XACT_TRACK_EVENT_STOP {
    
    
} XACT_TRACK_EVENT_STOP, *PXACT_TRACK_EVENT_STOP, *LPXACT_TRACK_EVENT_STOP;

typedef union XACT_EVENT_PLAY_DESC {

    struct {
        WORD wWaveIndex;
        WORD wBankIndex;
    } WaveSource;

    struct {
        DWORD dwMixBin;
    } EffectSource;

} XACT_EVENT_PLAY_DESC, *PXACT_EVENT_PLAY_DESC, *LPXACT_EVENT_PLAY_DESC;
 
typedef struct _XACT_TRACK_EVENT_PLAY {

	XACT_EVENT_PLAY_DESC PlayDesc;

} XACT_TRACK_EVENT_PLAY, *PXACT_TRACK_EVENT_PLAY, *LPXACT_TRACK_EVENT_PLAY;

typedef struct _XACT_EVENT_PITCH_VOLUME_VAR_DESC {

	struct {
		SHORT sPitchLo;
		SHORT sPitchHi;
	} Pitch;

	struct {
		SHORT sVolLo;
		SHORT sVolHi;
	} Volume;

} XACT_EVENT_PITCH_VOLUME_VAR_DESC, *PXACT_EVENT_PITCH_VOLUME_VAR_DESC, *LPXACT_EVENT_PITCH_VOLUME_VAR_DESC;

typedef struct _XACT_TRACK_EVENT_PITCH_VOLUME_VAR {

    XACT_EVENT_PITCH_VOLUME_VAR_DESC VarDesc;

} XACT_TRACK_EVENT_PITCH_VOLUME_VAR, *PXACT_TRACK_EVENT_PITCH_VOLUME_VAR, *LPXACT_TRACK_EVENT_PITCH_VOLUME_VAR;

typedef struct _XACT_TRACK_EVENT_PLAY_WITH_PITCH_VOLUME_VAR {

    XACT_EVENT_PLAY_DESC				PlayDesc;
	XACT_EVENT_PITCH_VOLUME_VAR_DESC	VarDesc;

} XACT_TRACK_EVENT_PLAY_WITH_PITCH_VOLUME_VAR, *PXACT_TRACK_EVENT_PLAY_WITH_PITCH_VOLUME_VAR, *LPXACT_TRACK_EVENT_PLAY_WITH_PITCH_VOLUME_VAR;

typedef struct _XACT_TRACK_EVENT_LOOPSTART {

	WORD	wLoopCount;

} XACT_TRACK_EVENT_LOOPSTART, *PXACT_TRACK_EVENT_LOOPSTART, *LPXACT_TRACK_EVENT_LOOPSTART;

typedef struct _XACT_TRACK_EVENT_LOOPEND {

} XACT_TRACK_EVENT_LOOPEND, *PXACT_TRACK_EVENT_LOOPEND, *LPXACT_TRACK_EVENT_LOOPEND;

union XACT_TRACK_EVENT_UNION {
    XACT_TRACK_EVENT_PLAY							Play;
	XACT_TRACK_EVENT_PLAY_WITH_PITCH_VOLUME_VAR		PlayWithPitchAndVolumeVariation;
    XACT_TRACK_EVENT_STOP							Stop;
	XACT_TRACK_EVENT_PITCH_VOLUME_VAR				PitchAndVolumeVariation;
	XACT_TRACK_EVENT_SETFREQUENCY					SetFrequency;
	XACT_TRACK_EVENT_SETVOLUME						SetVolume;
	XACT_TRACK_EVENT_SETHEADROOM					SetHeadroom;
	XACT_TRACK_EVENT_SETLFO							SetLFO;
	XACT_TRACK_EVENT_SETEG							SetEG;
	XACT_TRACK_EVENT_SETFILTER						SetFilter;
	XACT_TRACK_EVENT_SETEFFECTDATA					SetEffectData;
	XACT_TRACK_EVENT_MARKER							Marker;
	XACT_TRACK_EVENT_LOOPSTART						LoopStart;
	XACT_TRACK_EVENT_LOOPEND						LoopEnd;
	XACT_TRACK_EVENT_SETMIXBINVOLUMES				SetMixBinVolumes;
	
};

struct XACT_TRACK_EVENT {

    XACT_TRACK_EVENT_HEADER Header;
    XACT_TRACK_EVENT_UNION EventData;

};

//@@END_MSINTERNAL




//
// RXDK 5849 uplift: IXACTWmaPlayList.
//
// This is libxact's own copy of the API surface (the public shared/include/xact.h
// is shadowed by inc/xact.h on this lib's include path), so the playlist
// declarations have to be mirrored here or the engine cannot see the types it
// implements. Keep this in step with the public header -- same discipline as
// dsound.h/dsoundp.h.
//

typedef struct IXACTWmaPlayList IXACTWmaPlayList;
typedef IXACTWmaPlayList *LPXACTWMAPLAYLIST;
typedef IXACTWmaPlayList *PXACTWMAPLAYLIST;

typedef struct IXACTWmaSong IXACTWmaSong;
typedef IXACTWmaSong *LPXACTWMASONG;
typedef IXACTWmaSong *PXACTWMASONG;

typedef enum _XACT_WMA_PLAYLIST_ADD_TYPE
{
    eXACTWmaPlayListAdd_Directory = 1,
    eXACTWmaPlayListAdd_File,
    eXACTWmaPlayListAdd_Soundtrack,
    eXACTWmaPlayListAdd_SoundtrackSong,
    eXACTWmaPlayListAdd_Max
} XACT_WMA_PLAYLIST_ADD_TYPE;

#define XACT_FLAG_WMAPLAYLIST_PLAYBACK_RANDOM   0x00000001
#define XACT_FLAG_WMAPLAYLIST_PLAYBACK_LOOP     0x00000002
#define XACT_MASK_WMAPLAYLIST_PLAYBACK_FLAGS    (XACT_FLAG_WMAPLAYLIST_PLAYBACK_RANDOM | XACT_FLAG_WMAPLAYLIST_PLAYBACK_LOOP)

typedef struct _XACT_WMA_PLAYLIST_ADD {
    DWORD   dwType;
    PCSTR   pszFileName;
    DWORD   dwSoundtrackId;
    DWORD   dwSongId;
    DWORD   dwSongIndex;
} XACT_WMA_PLAYLIST_ADD, *PXACT_WMA_PLAYLIST_ADD;

typedef const XACT_WMA_PLAYLIST_ADD *PCXACT_WMA_PLAYLIST_ADD;

typedef struct _XACT_WMASONG_DESCRIPTION
{
    WMAXMOFileContDesc  Content;
    WMAXMOFileHeader    Header;
} XACT_WMASONG_DESCRIPTION, *PXACT_WMASONG_DESCRIPTION;

typedef struct _XACT_WMA_PLAYLIST_PROPERTIES {
    DWORD           dwPlaybackFlags;
    DWORD           dwSongEntryCount;
    PXACTWMASONG    pFirstSong;
    PXACTWMASONG    pLastSong;
} XACT_WMA_PLAYLIST_PROPERTIES, *PXACT_WMA_PLAYLIST_PROPERTIES;

STDAPI IXACTSoundBank_CreateWmaPlayList(PXACTSOUNDBANK pBank, DWORD dwSoundCueIndex, DWORD dwPlaybackFlags, PXACTWMAPLAYLIST *ppWmaPlayList);
STDAPI IXACTSoundBank_PauseSoundCue(PXACTSOUNDBANK pBank, PXACTSOUNDCUE pSoundCue, BOOL fPause);

STDAPI_(ULONG) IXACTWmaPlayList_AddRef(PXACTWMAPLAYLIST pPlayList);
STDAPI_(ULONG) IXACTWmaPlayList_Release(PXACTWMAPLAYLIST pPlayList);
STDAPI IXACTWmaPlayList_Add(PXACTWMAPLAYLIST pPlayList, PCXACT_WMA_PLAYLIST_ADD pDesc, PXACTWMASONG *ppSong);
STDAPI IXACTWmaPlayList_Remove(PXACTWMAPLAYLIST pPlayList, PXACTWMASONG pSong);
STDAPI IXACTWmaPlayList_SetCurrent(PXACTWMAPLAYLIST pPlayList, PXACTWMASONG pSong);
STDAPI IXACTWmaPlayList_Next(PXACTWMAPLAYLIST pPlayList);
STDAPI IXACTWmaPlayList_Previous(PXACTWMAPLAYLIST pPlayList);
STDAPI IXACTWmaPlayList_GetCurrentSongInfo(PXACTWMAPLAYLIST pPlayList, PDWORD pdwSongLength, PWCHAR pszNameBuffer, DWORD dwBufferSize, PXACTWMASONG *ppSong);
STDAPI IXACTWmaPlayList_GetCurrentSongInfoEx(PXACTWMAPLAYLIST pPlayList, PXACT_WMASONG_DESCRIPTION pDesc, PXACTWMASONG *ppSong);
STDAPI IXACTWmaPlayList_SetPlaybackBehavior(PXACTWMAPLAYLIST pPlayList, DWORD dwFlags);
STDAPI IXACTWmaPlayList_GetProperties(PXACTWMAPLAYLIST pPlayList, PXACT_WMA_PLAYLIST_PROPERTIES pProperties);

#ifdef __cplusplus

//
// Not a vtable interface: like IXACTSoundBank, this is a plain struct of inline
// forwarders, and the C entry points above cast the pointer back to the engine
// class. The engine class derives from it for the same reason CSoundBank does.
//
struct IXACTWmaPlayList
{
    __inline ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return IXACTWmaPlayList_AddRef(this);
    }

    __inline ULONG STDMETHODCALLTYPE Release(void)
    {
        return IXACTWmaPlayList_Release(this);
    }

    __inline HRESULT STDMETHODCALLTYPE Add(PCXACT_WMA_PLAYLIST_ADD pDesc, PXACTWMASONG *ppSong)
    {
        return IXACTWmaPlayList_Add(this, pDesc, ppSong);
    }

    __inline HRESULT STDMETHODCALLTYPE Remove(PXACTWMASONG pSong)
    {
        return IXACTWmaPlayList_Remove(this, pSong);
    }

    __inline HRESULT STDMETHODCALLTYPE SetCurrent(PXACTWMASONG pSong)
    {
        return IXACTWmaPlayList_SetCurrent(this, pSong);
    }

    __inline HRESULT STDMETHODCALLTYPE Next()
    {
        return IXACTWmaPlayList_Next(this);
    }

    __inline HRESULT STDMETHODCALLTYPE Previous()
    {
        return IXACTWmaPlayList_Previous(this);
    }

    __inline HRESULT STDMETHODCALLTYPE GetCurrentSongInfo(PDWORD pdwSongLength, PWCHAR pszNameBuffer, DWORD dwBufferSize, PXACTWMASONG *ppSong)
    {
        return IXACTWmaPlayList_GetCurrentSongInfo(this, pdwSongLength, pszNameBuffer, dwBufferSize, ppSong);
    }

    __inline HRESULT STDMETHODCALLTYPE GetCurrentSongInfoEx(PXACT_WMASONG_DESCRIPTION pDesc, PXACTWMASONG *ppSong)
    {
        return IXACTWmaPlayList_GetCurrentSongInfoEx(this, pDesc, ppSong);
    }

    __inline HRESULT STDMETHODCALLTYPE SetPlaybackBehavior(DWORD dwFlags)
    {
        return IXACTWmaPlayList_SetPlaybackBehavior(this, dwFlags);
    }

    __inline HRESULT STDMETHODCALLTYPE GetProperties(PXACT_WMA_PLAYLIST_PROPERTIES pProperties)
    {
        return IXACTWmaPlayList_GetProperties(this, pProperties);
    }
};

#endif // __cplusplus


#endif // __XACT_ENGINE INCLUDED__
