/**************************************************************************
 *
 *  Copyright (C) 2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       xact.h
 *  Content:    X-Box Audio Content Tool Runtime Engine.
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
#define XACT_MASK_NOTIFICATION_FLAGS	0xFFFF0000


#define XACT_NOTIFICATION_TYPE_UNUSED	0xFFFFFFFF

//
// flags used when registering notifications
//

#define XACT_FLAG_NOTIFICATION_PERSIST	0x00010000


typedef struct _XACT_NOTIFICATION_START {

    DWORD dwFlags;

} XACT_NOTIFICATION_START, *PXACT_NOTIFICATION_START, *LPXACT_NOTIFICATION_START;
 
typedef struct _XACT_NOTIFICATION_STOP {

    DWORD dwFlags;

} XACT_NOTIFICATION_STOP, *PXACT_NOTIFICATION_STOP, *LPXACT_NOTIFICATION_STOP;
 
typedef struct _XACT_NOTIFICATION_MARKER {

    BYTE    bData[XACT_SIZEOF_MARKER_DATA];

} XACT_NOTIFICATION_MARKER, *PXACT_NOTIFICATION_MARKER, *LPXACT_NOTIFICATION_MARKER;

union XACT_NOTIFICATION_UNION {

    XACT_NOTIFICATION_START Start;
    XACT_NOTIFICATION_STOP Stop;
    XACT_NOTIFICATION_MARKER Marker;

}; 

typedef struct _XACT_NOTIFICATION_DESCRIPTION{
    
    DWORD            dwType;
    PXACTSOUNDBANK   pSoundBank;
    PXACTSOUNDCUE    pSoundCue;	
	DWORD			 dwSoundCueIndex;
    PVOID            pvContext;
	HANDLE			 hEvent;

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
    PVOID pvHeap;
    DWORD dwHeapSize;
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

STDAPI XACTEngineCreate(PXACTENGINE *ppEngine, PXACT_RUNTIME_PARAMETERS pParams);
STDAPI_(void) XACTEngineDoWork(void);

STDAPI_(ULONG) IXACTEngine_AddRef(PXACTENGINE pEngine);
STDAPI_(ULONG) IXACTEngine_Release(PXACTENGINE pEngine);
STDAPI IXACTEngine_LoadDspImage(PXACTENGINE pEngine, PVOID pvData, DWORD dwSize, LPCDSEFFECTIMAGELOC pEffectLoc);
STDAPI IXACTEngine_CreateSoundSource(PXACTENGINE pEngine, DWORD dwFlags, PXACTSOUNDSOURCE *ppSoundSource);
STDAPI IXACTEngine_CreateSoundBank(PXACTENGINE pEngine, PVOID pvData, DWORD dwSize, PXACTSOUNDBANK *ppSoundBank);
STDAPI IXACTEngine_RegisterWaveBank(PXACTENGINE pEngine, PVOID pvData, DWORD dwSize, PXACTWAVEBANK * ppWaveBank);
STDAPI IXACTEngine_RegisterStreamedWaveBank(PXACTENGINE pEngine, PVOID pvStreamingBuffer, DWORD dwSize, HANDLE hFile, DWORD dwOffset, PXACTWAVEBANK *ppWaveBank);
STDAPI IXACTEngine_UnRegisterWaveBank(PXACTENGINE pEngine, PXACTWAVEBANK pWaveBank);
STDAPI IXACTEngine_SetMasterVolume(PXACTENGINE pEngine, LONG lVolume);
STDAPI IXACTEngine_SetListenerParameters(PXACTENGINE pEngine, LPCDS3DLISTENER pcDs3dListener, LPCDSI3DL2LISTENER pds3dl, DWORD dwApply);
STDAPI IXACTEngine_GlobalPause(PXACTENGINE pEngine, BOOL bPause);
STDAPI IXACTEngine_RegisterNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc);
STDAPI IXACTEngine_UnRegisterNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc);
STDAPI IXACTEngine_GetNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc, PXACT_NOTIFICATION pNotification);
STDAPI IXACTEngine_FlushNotification(PXACTENGINE pEngine, PXACT_NOTIFICATION_DESCRIPTION pNotificationDesc);
STDAPI IXACTEngine_CommitDeferredSettings(PXACTENGINE pEngine);

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

    __inline HRESULT STDMETHODCALLTYPE RegisterStreamedWaveBank(PVOID pvStreamingBuffer, DWORD dwSize, HANDLE hFile, DWORD dwOffset, PXACTWAVEBANK *ppWaveBank)
    {
        return IXACTEngine_RegisterStreamedWaveBank(this, pvStreamingBuffer, dwSize, hFile, dwOffset, ppWaveBank);
    }

    __inline HRESULT STDMETHODCALLTYPE UnRegisterWaveBank(PXACTWAVEBANK pWaveBank)
    {
        return IXACTEngine_UnRegisterWaveBank(this, pWaveBank);
    }

    __inline HRESULT STDMETHODCALLTYPE SetMasterVolume(LONG lVolume)
    {
        return IXACTEngine_SetMasterVolume(this, lVolume);
    }

    __inline HRESULT STDMETHODCALLTYPE SetListenerParameters(LPCDS3DLISTENER pcDs3dListener, LPCDSI3DL2LISTENER pds3dl, DWORD dwApply)
	{
	    return IXACTEngine_SetListenerParameters(this, pcDs3dListener, pds3dl, dwApply);
	}

    __inline HRESULT STDMETHODCALLTYPE GlobalPause(BOOL bPause)
    {
        return IXACTEngine_GlobalPause(this,bPause);
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





#endif // __XACT_ENGINE INCLUDED__
