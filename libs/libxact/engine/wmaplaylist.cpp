/***************************************************************************
 *
 *  File:       wmaplaylist.cpp
 *  Content:    IXACTWmaPlayList implementation.
 *
 *  RXDK 5849 uplift. Written from the 5849 public xact.h, not ported: the
 *  May-2020 leak has no WmaPlayList source at all. The songs themselves are
 *  decoded by the WMA file XMO in libdsound (WmaCreateDecoderEx).
 *
 *  WHAT WORKS: the playlist object itself -- building the song set (single file
 *  or a directory sweep), walking it in order or shuffled with or without
 *  looping, removing entries, and reading the current song's title and duration
 *  out of its WMA header.
 *
 *  WHAT DOES NOT: a playlist is bound to a sound cue, and playing THAT CUE is
 *  supposed to stream the current song. That path is not wired -- the cue plays
 *  whatever its wave bank says, as any other cue does, and the playlist is
 *  inert as far as audio is concerned. Wiring it means teaching CSoundCue to
 *  render from an XMO instead of a wave-bank entry, and pumping it from
 *  CEngine::DoWork; OpenCurrentDecoder above is the hook that path will use,
 *  which is why it exists with no caller yet. A title therefore links, runs,
 *  and can drive and display its playlist -- it just will not hear it.
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
      m_pszFileName(NULL)
{
    if (pszFileName != NULL) {
        DWORD cb = lstrlenA(pszFileName) + 1;
        m_pszFileName = (LPSTR)XactMemAlloc(cb, FALSE);
        if (m_pszFileName != NULL) {
            memcpy(m_pszFileName, pszFileName, cb);
        }
    }
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
      m_dwRandomSeed(0)
{
}

CWmaPlayList::~CWmaPlayList(void)
{
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

    // Append. Order is the order the title added them, which is what the
    // non-random playback mode walks.
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
    case eXACTWmaPlayListAdd_SoundtrackSong:
        // User soundtracks (CDs ripped to the hard disk) are enumerated through
        // xsndtrk.lib, which RXDK does not have -- the leak predates it and the
        // retail library is binary-only. Fail cleanly and specifically rather
        // than pretending to add nothing: a title that offers both its own
        // music and user soundtracks (as XActWMAPlayList does) can then fall
        // back to its own, instead of silently presenting an empty playlist.
        DPF_ERROR("User soundtracks need xsndtrk, which RXDK does not provide");
        return E_NOTIMPL;

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

    memset(&wfx, 0, sizeof(wfx));

    HRESULT hr = WmaCreateDecoderEx(m_pCurrent->GetFileName(),
                                    NULL,       // open by name
                                    FALSE,      // synchronous
                                    0,          // default lookahead
                                    0,          // default packet count
                                    0,          // default yield rate
                                    &wfx,
                                    &pDecoder);
    if (FAILED(hr)) {
        DPF_ERROR("Could not open %s (0x%08x)", m_pCurrent->GetFileName(), hr);
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
