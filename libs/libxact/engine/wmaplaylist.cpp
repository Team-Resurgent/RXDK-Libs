/***************************************************************************
 *
 *  File:       wmaplaylist.cpp
 *  Content:    IXACTWmaPlayList implementation.
 *
 *  RXDK 5849 uplift. Written from the 5849 public xact.h, not ported: the
 *  May-2020 leak has no WmaPlayList source at all. The songs themselves are
 *  decoded by the WMA file XMO in libdsound (WmaCreateDecoderEx).
 *
 *  WHAT WORKS: the playlist object itself -- building the song set (a single
 *  file, a directory sweep, or the songs the user ripped through the dash, via
 *  the soundtrack enumeration already in libxapi), walking it in order or
 *  shuffled with or without looping, removing entries, and reading the current
 *  song's title and duration.
 *
 *  Playback is at the bottom of this file. A playlist renders itself rather
 *  than going through a sound cue's wave-bank path -- its source is a file being
 *  decoded on the fly, not a bank entry -- so it owns a DirectSound stream,
 *  CSoundBank::Play/Stop divert to it for a cue that has a playlist bound, and
 *  CEngine::DoWork pumps it.
 *
 *  NOT hardware-tested: this builds and the packet bookkeeping is sound by
 *  inspection, but no audio has actually been heard from it.
 *
 ****************************************************************************/

#include "xacti.h"
#include "xboxdbg.h"

using namespace XACT;

//
// A playlist entry.
//

CWmaSong::CWmaSong(LPCSTR pszFileName)
    : m_pNext(NULL),
      m_pPrev(NULL),
      m_fSoundtrack(FALSE),
      m_pszFileName(NULL),
      m_dwSongId(0),
      m_dwSongLength(0)
{
    m_szName[0] = L'\0';

    if (pszFileName != NULL) {
        DWORD cb = lstrlenA(pszFileName) + 1;
        m_pszFileName = (LPSTR)XactMemAlloc(cb, FALSE);
        if (m_pszFileName != NULL) {
            memcpy(m_pszFileName, pszFileName, cb);
        }
    }
}

//
// A song the user ripped through the dash. Its name and duration came out of
// the soundtrack database during enumeration, so unlike a file song they are
// known without opening a decoder -- GetCurrentSongInfo can answer for these
// without the file-open cost.
//
CWmaSong::CWmaSong(DWORD dwSongId, DWORD dwSongLength, LPCWSTR pszName)
    : m_pNext(NULL),
      m_pPrev(NULL),
      m_fSoundtrack(TRUE),
      m_pszFileName(NULL),
      m_dwSongId(dwSongId),
      m_dwSongLength(dwSongLength)
{
    DWORD i = 0;
    if (pszName != NULL) {
        while (pszName[i] != L'\0' && i + 1 < MAX_SONG_NAME) {
            m_szName[i] = pszName[i];
            i++;
        }
    }
    m_szName[i] = L'\0';
}

CWmaSong::~CWmaSong(void)
{
    if (m_pszFileName != NULL) {
        XactMemFree(m_pszFileName);
    }
}


//
// The playlist.
//

CWmaPlayList::CWmaPlayList(void)
    : m_pSoundBank(NULL),
      m_dwSoundCueIndex(0),
      m_dwPlaybackFlags(0),
      m_pFirst(NULL),
      m_pLast(NULL),
      m_pCurrent(NULL),
      m_dwSongCount(0),
      m_pDecoder(NULL),
      m_pDecoderSong(NULL),
      m_dwRandomSeed(0),
      m_pStream(NULL),
      m_fPlaying(FALSE),
      m_dwPacketSize(0),
      m_fSongEnded(FALSE)
{
    for (DWORD i = 0; i < PACKET_COUNT; i++) {
        m_apvPacket[i] = NULL;
        m_adwStatus[i] = XMEDIAPACKET_STATUS_SUCCESS;
    }
    InitializeListHead(&m_ListEntry);
}

CWmaPlayList::~CWmaPlayList(void)
{
    // Unregister first: the engine's DoWork walks this list, and a half-torn
    // playlist must not still be on it.
    if (g_pEngine != NULL) {
        g_pEngine->UnregisterPlayList(this);
    }

    StopPlayback();
    CloseDecoder();

    CWmaSong *pSong = m_pFirst;
    while (pSong != NULL) {
        CWmaSong *pNext = pSong->m_pNext;
        delete pSong;
        pSong = pNext;
    }
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::Initialize"

HRESULT CWmaPlayList::Initialize(CSoundBank *pSoundBank, DWORD dwSoundCueIndex, DWORD dwPlaybackFlags)
{
    if (dwPlaybackFlags & ~XACT_MASK_WMAPLAYLIST_PLAYBACK_FLAGS) {
        DPF_ERROR("Unknown playback flags (0x%08x)", dwPlaybackFlags);
        return E_INVALIDARG;
    }

    m_pSoundBank      = pSoundBank;
    m_dwSoundCueIndex = dwSoundCueIndex;
    m_dwPlaybackFlags = dwPlaybackFlags;

    // The shuffle only has to be unpredictable to a listener, not to an
    // attacker, and the Xbox tick count at playlist creation is as good a
    // starting point as anything available this early.
    m_dwRandomSeed = GetTickCount() | 1;

    // The engine keeps a weak list of live playlists so CSoundBank::Play can
    // find the one bound to a cue, and so DoWork can pump them. Weak on purpose:
    // the title owns the playlist's lifetime through Release, and the
    // destructor unregisters.
    if (g_pEngine != NULL) {
        g_pEngine->RegisterPlayList(this);
    }

    return S_OK;
}


ULONG STDMETHODCALLTYPE CWmaPlayList::AddRef(void)
{
    return CRefCount::AddRef();
}

ULONG STDMETHODCALLTYPE CWmaPlayList::Release(void)
{
    return CRefCount::Release();
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::AddFile"

HRESULT CWmaPlayList::AddFile(LPCSTR pszFileName, PXACTWMASONG *ppSong)
{
    if (pszFileName == NULL) {
        return E_INVALIDARG;
    }

    CWmaSong *pSong = new CWmaSong(pszFileName);
    if (pSong == NULL) {
        return E_OUTOFMEMORY;
    }
    if (!pSong->IsValid()) {
        delete pSong;
        return E_OUTOFMEMORY;
    }

    return LinkSong(pSong, ppSong);
}


//
// Append. Order is the order the title added them, which is what the
// non-random playback mode walks.
//
HRESULT CWmaPlayList::LinkSong(CWmaSong *pSong, PXACTWMASONG *ppSong)
{
    pSong->m_pPrev = m_pLast;
    if (m_pLast != NULL) {
        m_pLast->m_pNext = pSong;
    } else {
        m_pFirst = pSong;
    }
    m_pLast = pSong;
    m_dwSongCount++;

    if (ppSong != NULL) {
        *ppSong = (PXACTWMASONG)pSong;
    }

    return S_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::AddDirectory"

HRESULT CWmaPlayList::AddDirectory(LPCSTR pszDirectory, PXACTWMASONG *ppSong)
{
    CHAR                szPattern[MAX_PATH];
    CHAR                szPath[MAX_PATH];
    WIN32_FIND_DATAA    fd;
    HANDLE              hFind;
    HRESULT             hr    = S_OK;
    DWORD               cbDir;
    BOOL                fSlash;
    PXACTWMASONG        pFirstAdded = NULL;

    if (pszDirectory == NULL) {
        return E_INVALIDARG;
    }

    cbDir = lstrlenA(pszDirectory);
    if (cbDir == 0 || cbDir + sizeof("\\*.wma") > MAX_PATH) {
        return E_INVALIDARG;
    }

    fSlash = (pszDirectory[cbDir - 1] == '\\');

    lstrcpyA(szPattern, pszDirectory);
    lstrcatA(szPattern, fSlash ? "*.wma" : "\\*.wma");

    hFind = FindFirstFileA(szPattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        // An empty or absent directory is not an error -- the playlist simply
        // gains nothing. A title that cares can see it in GetProperties.
        return S_FALSE;
    }

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        if (cbDir + 1 + lstrlenA(fd.cFileName) + 1 > MAX_PATH) {
            DPF_ERROR("Path too long, skipped: %s", fd.cFileName);
            continue;
        }

        lstrcpyA(szPath, pszDirectory);
        if (!fSlash) {
            lstrcatA(szPath, "\\");
        }
        lstrcatA(szPath, fd.cFileName);

        PXACTWMASONG pAdded = NULL;
        hr = AddFile(szPath, &pAdded);
        if (FAILED(hr)) {
            break;
        }
        if (pFirstAdded == NULL) {
            pFirstAdded = pAdded;
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);

    // Add hands back ONE song, so for a directory that is the first of the
    // batch -- the rest are reachable by walking from it.
    if (SUCCEEDED(hr) && ppSong != NULL) {
        *ppSong = pFirstAdded;
    }

    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::AddSoundtrackSong"

//
// One song out of a soundtrack the user ripped through the dash.
//
// The database gives us the song's id, duration and name here, so nothing has
// to be opened until the song is actually selected.
//
HRESULT CWmaPlayList::AddSoundtrackSong(DWORD dwSoundtrackId, DWORD dwSongIndex, PXACTWMASONG *ppSong)
{
    DWORD   dwSongId     = 0;
    DWORD   dwSongLength = 0;
    WCHAR   szName[MAX_SONG_NAME];

    szName[0] = L'\0';

    if (!XGetSoundtrackSongInfo(dwSoundtrackId, dwSongIndex, &dwSongId, &dwSongLength,
                                szName, sizeof(szName))) {
        DPF_ERROR("No song %d in soundtrack %d", dwSongIndex, dwSoundtrackId);
        return E_INVALIDARG;
    }

    CWmaSong *pSong = new CWmaSong(dwSongId, dwSongLength, szName);
    if (pSong == NULL) {
        return E_OUTOFMEMORY;
    }

    return LinkSong(pSong, ppSong);
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::AddSoundtrack"

//
// Every song in one ripped soundtrack.
//
// XGetSoundtrackSongInfo is indexed rather than counted, so walk until it says
// there is no song at that index. That also means a soundtrack the user deleted
// between enumeration and here simply adds nothing rather than failing.
//
HRESULT CWmaPlayList::AddSoundtrack(DWORD dwSoundtrackId, PXACTWMASONG *ppSong)
{
    PXACTWMASONG    pFirstAdded = NULL;
    DWORD           dwIndex     = 0;

    for (;;) {
        PXACTWMASONG pAdded = NULL;

        HRESULT hr = AddSoundtrackSong(dwSoundtrackId, dwIndex, &pAdded);
        if (hr == E_INVALIDARG) {
            break;      // past the last song
        }
        if (FAILED(hr)) {
            return hr;  // out of memory -- a real failure
        }

        if (pFirstAdded == NULL) {
            pFirstAdded = pAdded;
        }
        dwIndex++;

        // The database caps a soundtrack at this many songs; stop rather than
        // spin if it ever hands back success forever.
        if (dwIndex >= MAX_SONGS_IN_SNDTRK) {
            break;
        }
    }

    if (dwIndex == 0) {
        DPF_ERROR("Soundtrack %d has no songs", dwSoundtrackId);
        return S_FALSE;
    }

    // Like a directory add, hand back the first of the batch.
    if (ppSong != NULL) {
        *ppSong = pFirstAdded;
    }

    return S_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::Add"

HRESULT STDMETHODCALLTYPE CWmaPlayList::Add(PCXACT_WMA_PLAYLIST_ADD pDesc, PXACTWMASONG *ppSong)
{
    if (pDesc == NULL) {
        return E_INVALIDARG;
    }

    if (ppSong != NULL) {
        *ppSong = NULL;
    }

    switch (pDesc->dwType) {

    case eXACTWmaPlayListAdd_File:
        return AddFile(pDesc->pszFileName, ppSong);

    case eXACTWmaPlayListAdd_Directory:
        return AddDirectory(pDesc->pszFileName, ppSong);

    case eXACTWmaPlayListAdd_Soundtrack:
        return AddSoundtrack(pDesc->dwSoundtrackId, ppSong);

    case eXACTWmaPlayListAdd_SoundtrackSong:
        return AddSoundtrackSong(pDesc->dwSoundtrackId, pDesc->dwSongIndex, ppSong);

    default:
        DPF_ERROR("Unknown add type (%d)", pDesc->dwType);
        return E_INVALIDARG;
    }
}


VOID CWmaPlayList::Unlink(CWmaSong *pSong)
{
    if (pSong->m_pPrev != NULL) {
        pSong->m_pPrev->m_pNext = pSong->m_pNext;
    } else {
        m_pFirst = pSong->m_pNext;
    }

    if (pSong->m_pNext != NULL) {
        pSong->m_pNext->m_pPrev = pSong->m_pPrev;
    } else {
        m_pLast = pSong->m_pPrev;
    }

    m_dwSongCount--;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::Remove"

HRESULT STDMETHODCALLTYPE CWmaPlayList::Remove(PXACTWMASONG pSong)
{
    CWmaSong *p = (CWmaSong *)pSong;

    if (p == NULL) {
        return E_INVALIDARG;
    }

    // Removing the song that is playing has to leave a valid current song, or
    // the next Next()/Play() would walk off a freed node. Step to the
    // neighbour first.
    if (m_pCurrent == p) {
        m_pCurrent = (p->m_pNext != NULL) ? p->m_pNext : p->m_pPrev;
    }
    if (m_pDecoderSong == p) {
        CloseDecoder();
    }

    Unlink(p);
    delete p;

    return S_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::SetCurrent"

HRESULT STDMETHODCALLTYPE CWmaPlayList::SetCurrent(PXACTWMASONG pSong)
{
    CWmaSong *p = (CWmaSong *)pSong;

    if (p == NULL) {
        return E_INVALIDARG;
    }

    if (m_pCurrent != p) {
        m_pCurrent = p;
        CloseDecoder();     // the open decoder belongs to the old song
    }

    return S_OK;
}


//
// A shuffle step. Deliberately allowed to land on the song already playing when
// the list is short -- the alternative (never repeat) turns a two-song playlist
// into strict alternation, which is not what random means.
//
CWmaSong * CWmaPlayList::PickRandom(void) const
{
    if (m_dwSongCount == 0) {
        return NULL;
    }

    // Park-Miller-ish LCG; the sequence only has to look arbitrary.
    DWORD dwNext = m_dwRandomSeed * 1103515245 + 12345;
    ((CWmaPlayList *)this)->m_dwRandomSeed = dwNext;

    DWORD dwIndex = (dwNext >> 16) % m_dwSongCount;

    CWmaSong *p = m_pFirst;
    while (dwIndex-- > 0 && p != NULL) {
        p = p->m_pNext;
    }

    return p;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::Next"

HRESULT STDMETHODCALLTYPE CWmaPlayList::Next(void)
{
    if (m_dwSongCount == 0) {
        return E_FAIL;
    }

    CWmaSong *pNext;

    if (m_dwPlaybackFlags & XACT_FLAG_WMAPLAYLIST_PLAYBACK_RANDOM) {
        pNext = PickRandom();
    } else if (m_pCurrent == NULL) {
        // No song selected yet: Next() is how a title asks for the first one.
        pNext = m_pFirst;
    } else if (m_pCurrent->m_pNext != NULL) {
        pNext = m_pCurrent->m_pNext;
    } else if (m_dwPlaybackFlags & XACT_FLAG_WMAPLAYLIST_PLAYBACK_LOOP) {
        pNext = m_pFirst;
    } else {
        // Past the end of a non-looping playlist. Leave the current song alone
        // so a title that ignores the result does not restart the last track.
        return S_FALSE;
    }

    if (pNext == NULL) {
        return E_FAIL;
    }

    if (pNext != m_pCurrent) {
        m_pCurrent = pNext;
        CloseDecoder();
    }

    return S_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::Previous"

HRESULT STDMETHODCALLTYPE CWmaPlayList::Previous(void)
{
    if (m_dwSongCount == 0) {
        return E_FAIL;
    }

    CWmaSong *pPrev;

    if (m_dwPlaybackFlags & XACT_FLAG_WMAPLAYLIST_PLAYBACK_RANDOM) {
        pPrev = PickRandom();
    } else if (m_pCurrent == NULL) {
        pPrev = m_pLast;
    } else if (m_pCurrent->m_pPrev != NULL) {
        pPrev = m_pCurrent->m_pPrev;
    } else if (m_dwPlaybackFlags & XACT_FLAG_WMAPLAYLIST_PLAYBACK_LOOP) {
        pPrev = m_pLast;
    } else {
        return S_FALSE;
    }

    if (pPrev == NULL) {
        return E_FAIL;
    }

    if (pPrev != m_pCurrent) {
        m_pCurrent = pPrev;
        CloseDecoder();
    }

    return S_OK;
}


VOID CWmaPlayList::CloseDecoder(void)
{
    if (m_pDecoder != NULL) {
        m_pDecoder->Release();
        m_pDecoder = NULL;
    }
    m_pDecoderSong = NULL;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::EnsureDecoder"

//
// Open a decoder on the current song if one is not already open.
//
// This is where GetCurrentSongInfo's documented cost comes from: the metadata
// lives in the file's header, so the file has to be opened and parsed before
// there is anything to report. Once open the decoder is kept, which is why the
// XDK documents repeat calls as fast and the call right after Next() as slow.
//
HRESULT CWmaPlayList::EnsureDecoder(void)
{
    if (m_pCurrent == NULL) {
        return E_FAIL;
    }

    if (m_pDecoder != NULL && m_pDecoderSong == m_pCurrent) {
        return S_OK;
    }

    CloseDecoder();

    XWmaFileMediaObject *pDecoder = NULL;
    WAVEFORMATEX         wfx;
    LPCSTR               pszFileName = NULL;
    HANDLE               hFile       = NULL;

    memset(&wfx, 0, sizeof(wfx));

    if (m_pCurrent->IsSoundtrackSong()) {
        // A ripped song has no path a title could name -- the soundtrack
        // database hands out a handle instead. WmaCreateDecoderEx takes either.
        hFile = XOpenSoundtrackSong(m_pCurrent->GetSongId(), FALSE);
        if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
            DPF_ERROR("Could not open soundtrack song %d", m_pCurrent->GetSongId());
            return E_FAIL;
        }
    } else {
        pszFileName = m_pCurrent->GetFileName();
    }

    HRESULT hr = WmaCreateDecoderEx(pszFileName,
                                    hFile,
                                    FALSE,      // synchronous
                                    0,          // default lookahead
                                    0,          // default packet count
                                    0,          // default yield rate
                                    &wfx,
                                    &pDecoder);
    if (FAILED(hr)) {
        // The decoder took no ownership of a handle it could not use.
        if (hFile != NULL) {
            CloseHandle(hFile);
        }
        DPF_ERROR("Could not open song (0x%08x)", hr);
        return hr;
    }

    m_pDecoder     = pDecoder;
    m_pDecoderSong = m_pCurrent;

    return S_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::OpenCurrentDecoder"

HRESULT CWmaPlayList::OpenCurrentDecoder(XWmaFileMediaObject **ppDecoder)
{
    if (ppDecoder == NULL) {
        return E_INVALIDARG;
    }

    *ppDecoder = NULL;

    HRESULT hr = EnsureDecoder();
    if (FAILED(hr)) {
        return hr;
    }

    // Borrowed, not addrefed -- the playlist owns the decoder and closes it
    // when the current song changes. Matches how the XMO family hands back
    // audio streams elsewhere.
    *ppDecoder = m_pDecoder;

    return S_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::GetCurrentSongInfo"

HRESULT STDMETHODCALLTYPE CWmaPlayList::GetCurrentSongInfo(PDWORD pdwSongLength, PWCHAR pszNameBuffer,
                                                           DWORD dwBufferSize, PXACTWMASONG *ppSong)
{
    if (ppSong != NULL) {
        *ppSong = NULL;
    }

    // A ripped song's name and duration came from the soundtrack database when
    // it was added, so answer from there and skip opening the file entirely --
    // this is the fast path the XDK documents for repeat calls, available here
    // even on the first one.
    if (m_pCurrent != NULL && m_pCurrent->IsSoundtrackSong()) {
        if (pdwSongLength != NULL) {
            *pdwSongLength = m_pCurrent->GetSongLength();
        }
        if (pszNameBuffer != NULL && dwBufferSize >= sizeof(WCHAR)) {
            DWORD   cchBuffer = dwBufferSize / sizeof(WCHAR);
            LPCWSTR pszName   = m_pCurrent->GetName();
            DWORD   i         = 0;

            while (pszName[i] != L'\0' && i + 1 < cchBuffer) {
                pszNameBuffer[i] = pszName[i];
                i++;
            }
            pszNameBuffer[i] = L'\0';
        }
        if (ppSong != NULL) {
            *ppSong = (PXACTWMASONG)m_pCurrent;
        }
        return S_OK;
    }

    HRESULT hr = EnsureDecoder();
    if (FAILED(hr)) {
        return hr;
    }

    if (pdwSongLength != NULL) {
        WMAXMOFileHeader header;
        memset(&header, 0, sizeof(header));

        hr = m_pDecoder->GetFileHeader(&header);
        if (FAILED(hr)) {
            return hr;
        }
        *pdwSongLength = header.dwDuration;
    }

    if (pszNameBuffer != NULL && dwBufferSize >= sizeof(WCHAR)) {
        WMAXMOFileContDesc desc;
        memset(&desc, 0, sizeof(desc));

        hr = m_pDecoder->GetFileContentDescription(&desc);
        if (FAILED(hr)) {
            return hr;
        }

        // dwBufferSize is a BYTE count (the sample passes
        // MAX_SONG_NAME * sizeof(WCHAR)), so convert before indexing.
        DWORD cchBuffer = dwBufferSize / sizeof(WCHAR);
        DWORD cchTitle  = desc.wTitleLength;

        if (desc.pTitle == NULL || cchTitle == 0) {
            // No title tag. Better to show the file name than nothing.
            LPCSTR pszFile = m_pCurrent->GetFileName();
            LPCSTR pszLeaf = pszFile;
            for (LPCSTR p = pszFile; *p != '\0'; p++) {
                if (*p == '\\' || *p == '/') {
                    pszLeaf = p + 1;
                }
            }

            DWORD i = 0;
            while (pszLeaf[i] != '\0' && i + 1 < cchBuffer) {
                pszNameBuffer[i] = (WCHAR)(BYTE)pszLeaf[i];
                i++;
            }
            pszNameBuffer[i] = L'\0';
        } else {
            if (cchTitle + 1 > cchBuffer) {
                cchTitle = cchBuffer - 1;
            }
            memcpy(pszNameBuffer, desc.pTitle, cchTitle * sizeof(WCHAR));
            pszNameBuffer[cchTitle] = L'\0';
        }
    }

    if (ppSong != NULL) {
        *ppSong = (PXACTWMASONG)m_pCurrent;
    }

    return S_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::GetCurrentSongInfoEx"

HRESULT STDMETHODCALLTYPE CWmaPlayList::GetCurrentSongInfoEx(PXACT_WMASONG_DESCRIPTION pDesc,
                                                             PXACTWMASONG *ppSong)
{
    if (ppSong != NULL) {
        *ppSong = NULL;
    }

    if (pDesc == NULL) {
        return E_INVALIDARG;
    }

    HRESULT hr = EnsureDecoder();
    if (FAILED(hr)) {
        return hr;
    }

    memset(pDesc, 0, sizeof(*pDesc));

    hr = m_pDecoder->GetFileContentDescription(&pDesc->Content);
    if (SUCCEEDED(hr)) {
        hr = m_pDecoder->GetFileHeader(&pDesc->Header);
    }
    if (FAILED(hr)) {
        return hr;
    }

    // The string pointers in Content aim into the decoder's own parsed header,
    // so they stay valid exactly as long as the current song does -- the next
    // Next()/SetCurrent() closes that decoder. Same lifetime the retail library
    // gives them.
    if (ppSong != NULL) {
        *ppSong = (PXACTWMASONG)m_pCurrent;
    }

    return S_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::SetPlaybackBehavior"

HRESULT STDMETHODCALLTYPE CWmaPlayList::SetPlaybackBehavior(DWORD dwFlags)
{
    if (dwFlags & ~XACT_MASK_WMAPLAYLIST_PLAYBACK_FLAGS) {
        DPF_ERROR("Unknown playback flags (0x%08x)", dwFlags);
        return E_INVALIDARG;
    }

    m_dwPlaybackFlags = dwFlags;

    return S_OK;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::GetProperties"

HRESULT STDMETHODCALLTYPE CWmaPlayList::GetProperties(PXACT_WMA_PLAYLIST_PROPERTIES pProperties)
{
    if (pProperties == NULL) {
        return E_INVALIDARG;
    }

    pProperties->dwPlaybackFlags  = m_dwPlaybackFlags;
    pProperties->dwSongEntryCount = m_dwSongCount;
    pProperties->pFirstSong       = (PXACTWMASONG)m_pFirst;
    pProperties->pLastSong        = (PXACTWMASONG)m_pLast;

    return S_OK;
}


//===========================================================================
//
//  Playback.
//
//  A playlist does not render through a sound cue's wave-bank path, because its
//  source is a WMA file being decoded on the fly rather than a bank entry. It
//  owns a DirectSound stream and feeds it from the decoder, and the engine pumps
//  it from DoWork alongside the cues. CSoundBank::Play/Stop divert here when the
//  cue they were handed has a playlist bound to it.
//
//===========================================================================

#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::OpenStreamForCurrentSong"

HRESULT CWmaPlayList::OpenStreamForCurrentSong(void)
{
    HRESULT hr = EnsureDecoder();
    if (FAILED(hr)) {
        return hr;
    }

    XMEDIAINFO info;
    memset(&info, 0, sizeof(info));

    hr = m_pDecoder->GetInfo(&info);
    if (FAILED(hr)) {
        return hr;
    }

    // The decoder reports its PCM format through the WAVEFORMATEX it filled in
    // at create time; ask it again here rather than caching, since the next song
    // may differ in rate or channel count.
    WAVEFORMATEX wfx;
    memset(&wfx, 0, sizeof(wfx));

    WMAXMOFileHeader header;
    memset(&header, 0, sizeof(header));
    hr = m_pDecoder->GetFileHeader(&header);
    if (FAILED(hr)) {
        return hr;
    }

    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = (WORD)header.dwNumChannels;
    wfx.nSamplesPerSec  = header.dwSampleRate;
    wfx.wBitsPerSample  = 16;                       // the WMA XMO decodes to 16-bit
    wfx.nBlockAlign     = (WORD)(wfx.nChannels * wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (wfx.nChannels == 0 || wfx.nSamplesPerSec == 0) {
        DPF_ERROR("Song header reports no format (%d ch, %d Hz)",
                  header.dwNumChannels, header.dwSampleRate);
        return E_FAIL;
    }

    // XMO_STREAMF_FIXED_SAMPLE_SIZE means dwOutputSize is ONE sample frame, not
    // a buffer size, so a packet has to be a multiple of it rather than equal to
    // it. Pick something around a tenth of a second so the ring is a sensible
    // fraction of a second deep without being wasteful.
    DWORD dwFrame = (info.dwFlags & XMO_STREAMF_FIXED_SAMPLE_SIZE) && info.dwOutputSize
                        ? info.dwOutputSize
                        : wfx.nBlockAlign;
    if (dwFrame == 0) {
        dwFrame = wfx.nBlockAlign;
    }

    m_dwPacketSize = (wfx.nAvgBytesPerSec / 10);
    m_dwPacketSize -= (m_dwPacketSize % dwFrame);
    if (m_dwPacketSize == 0) {
        m_dwPacketSize = dwFrame;
    }

    DSSTREAMDESC dssd;
    memset(&dssd, 0, sizeof(dssd));
    dssd.dwMaxAttachedPackets = PACKET_COUNT;
    dssd.lpwfxFormat          = &wfx;

    hr = DirectSoundCreateStream(&dssd, &m_pStream);
    if (FAILED(hr)) {
        DPF_ERROR("Could not create the playlist stream (0x%08x)", hr);
        return hr;
    }

    for (DWORD i = 0; i < PACKET_COUNT; i++) {
        m_apvPacket[i] = XactMemAlloc(m_dwPacketSize, FALSE);
        if (m_apvPacket[i] == NULL) {
            CloseStream();
            return E_OUTOFMEMORY;
        }
        m_adwStatus[i] = XMEDIAPACKET_STATUS_SUCCESS;   // free
    }

    m_fSongEnded = FALSE;

    return S_OK;
}


VOID CWmaPlayList::CloseStream(void)
{
    if (m_pStream != NULL) {
        m_pStream->Flush();
        m_pStream->Release();
        m_pStream = NULL;
    }

    for (DWORD i = 0; i < PACKET_COUNT; i++) {
        if (m_apvPacket[i] != NULL) {
            XactMemFree(m_apvPacket[i]);
            m_apvPacket[i] = NULL;
        }
        m_adwStatus[i] = XMEDIAPACKET_STATUS_SUCCESS;
    }

    m_dwPacketSize = 0;
    m_fSongEnded   = FALSE;
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::SubmitPackets"

//
// Fill every free slot in the ring from the decoder.
//
VOID CWmaPlayList::SubmitPackets(void)
{
    if (m_pStream == NULL || m_pDecoder == NULL || m_fSongEnded) {
        return;
    }

    for (DWORD i = 0; i < PACKET_COUNT; i++) {

        // PENDING means DirectSound still owns this slot.
        if (m_adwStatus[i] == XMEDIAPACKET_STATUS_PENDING) {
            continue;
        }

        XMEDIAPACKET xmp;
        DWORD        dwDecoded = 0;

        memset(&xmp, 0, sizeof(xmp));
        xmp.pvBuffer         = m_apvPacket[i];
        xmp.dwMaxSize        = m_dwPacketSize;
        xmp.pdwCompletedSize = &dwDecoded;

        // Pull PCM out of the decoder.
        HRESULT hr = m_pDecoder->Process(NULL, &xmp);
        if (FAILED(hr) || dwDecoded == 0) {
            // Out of data: the song is done. Do NOT advance here -- the stream
            // still has queued packets to play, and cutting to the next song now
            // would clip the tail. DoWork advances once the ring drains.
            m_fSongEnded = TRUE;
            return;
        }

        XMEDIAPACKET out;
        memset(&out, 0, sizeof(out));
        out.pvBuffer  = m_apvPacket[i];
        out.dwMaxSize = dwDecoded;
        out.pdwStatus = &m_adwStatus[i];

        m_adwStatus[i] = XMEDIAPACKET_STATUS_PENDING;

        hr = m_pStream->Process(&out, NULL);
        if (FAILED(hr)) {
            m_adwStatus[i] = XMEDIAPACKET_STATUS_FAILURE;
            DPF_ERROR("Stream rejected a packet (0x%08x)", hr);
            return;
        }
    }
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::StartPlayback"

HRESULT CWmaPlayList::StartPlayback(void)
{
    if (m_fPlaying) {
        return S_FALSE;
    }

    if (m_pCurrent == NULL) {
        // Nothing selected yet. Playing a playlist without having chosen a song
        // starts at the beginning, which is what Next() from empty does.
        HRESULT hr = Next();
        if (FAILED(hr)) {
            return hr;
        }
    }

    HRESULT hr = OpenStreamForCurrentSong();
    if (FAILED(hr)) {
        return hr;
    }

    SubmitPackets();

    hr = m_pStream->Pause(DSSTREAMPAUSE_RESUME);
    if (FAILED(hr)) {
        CloseStream();
        return hr;
    }

    m_fPlaying = TRUE;

    return S_OK;
}


VOID CWmaPlayList::StopPlayback(void)
{
    m_fPlaying = FALSE;
    CloseStream();
}


#undef DPF_FNAME
#define DPF_FNAME "CWmaPlayList::DoWork"

VOID CWmaPlayList::DoWork(void)
{
    if (!m_fPlaying || m_pStream == NULL) {
        return;
    }

    if (!m_fSongEnded) {
        SubmitPackets();
        return;
    }

    // The decoder is exhausted. Wait for the queued packets to finish before
    // moving on, so the end of the song is actually heard.
    for (DWORD i = 0; i < PACKET_COUNT; i++) {
        if (m_adwStatus[i] == XMEDIAPACKET_STATUS_PENDING) {
            return;
        }
    }

    // Song finished. Advance and start the next one; Next() already applies the
    // random and loop behaviour, and returns S_FALSE at the end of a playlist
    // that is not looping.
    CloseStream();

    HRESULT hr = Next();
    if (hr != S_OK) {
        m_fPlaying = FALSE;     // end of a non-looping playlist
        return;
    }

    if (FAILED(OpenStreamForCurrentSong())) {
        m_fPlaying = FALSE;
        return;
    }

    SubmitPackets();

    if (FAILED(m_pStream->Pause(DSSTREAMPAUSE_RESUME))) {
        StopPlayback();
    }
}
