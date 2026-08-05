/***************************************************************************
 *
 *  File:       wmaplaylist.h
 *  Content:    IXACTWmaPlayList -- an ordered set of .wma songs bound to a
 *              sound cue, which plays them in turn.
 *
 *  RXDK 5849 uplift. This is NOT a port: the May-2020 leak contains no
 *  WmaPlayList source anywhere (grep-confirmed across the whole vendor tree),
 *  because the feature postdates it. Written against the 5849 public xact.h,
 *  over the WMA file XMO that libdsound now provides.
 *
 ****************************************************************************/

#ifndef __WMAPLAYLIST_H__
#define __WMAPLAYLIST_H__

namespace XACT
{

class CWmaPlayList;

//
// One entry in a playlist.
//
// A title only ever sees this as the opaque PXACTWMASONG that Add hands back
// and SetCurrent/Remove take again, so the layout is ours to choose. The songs
// form a doubly-linked list rather than an array because Remove is part of the
// API and must not invalidate the handles a title is still holding.
//
// A song is either a file the title shipped or one the user ripped through the
// dash. They differ only in how the decoder is opened -- by path, or by a song
// id that XOpenSoundtrackSong turns into a handle -- and WmaCreateDecoderEx
// accepts either, so everything downstream is common.
class CWmaSong
{
public:
    CWmaSong(LPCSTR pszFileName);
    CWmaSong(DWORD dwSongId, DWORD dwSongLength, LPCWSTR pszName);
    ~CWmaSong(void);

    BOOL        IsSoundtrackSong(void) const { return m_fSoundtrack; }
    LPCSTR      GetFileName(void) const      { return m_pszFileName; }
    DWORD       GetSongId(void) const        { return m_dwSongId; }
    DWORD       GetSongLength(void) const     { return m_dwSongLength; }
    LPCWSTR     GetName(void) const          { return m_fSoundtrack ? m_szName : NULL; }

    BOOL        IsValid(void) const
    {
        return m_fSoundtrack || m_pszFileName != NULL;
    }

private:
    friend class CWmaPlayList;

    CWmaSong *  m_pNext;
    CWmaSong *  m_pPrev;

    BOOL        m_fSoundtrack;
    LPSTR       m_pszFileName;      // file songs
    DWORD       m_dwSongId;         // soundtrack songs
    DWORD       m_dwSongLength;
    WCHAR       m_szName[MAX_SONG_NAME];
};

//
// The playlist itself.
//
class CWmaPlayList
    : public IXACTWmaPlayList, public CRefCount
{
public:
    CWmaPlayList(void);
    ~CWmaPlayList(void);

    HRESULT Initialize(CSoundBank *pSoundBank, DWORD dwSoundCueIndex, DWORD dwPlaybackFlags);

    // IXACTWmaPlayList
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    HRESULT STDMETHODCALLTYPE Add(PCXACT_WMA_PLAYLIST_ADD pDesc, PXACTWMASONG *ppSong);
    HRESULT STDMETHODCALLTYPE Remove(PXACTWMASONG pSong);
    HRESULT STDMETHODCALLTYPE SetCurrent(PXACTWMASONG pSong);
    HRESULT STDMETHODCALLTYPE Next(void);
    HRESULT STDMETHODCALLTYPE Previous(void);
    HRESULT STDMETHODCALLTYPE GetCurrentSongInfo(PDWORD pdwSongLength, PWCHAR pszNameBuffer,
                                                 DWORD dwBufferSize, PXACTWMASONG *ppSong);
    HRESULT STDMETHODCALLTYPE GetCurrentSongInfoEx(PXACT_WMASONG_DESCRIPTION pDesc,
                                                   PXACTWMASONG *ppSong);
    HRESULT STDMETHODCALLTYPE SetPlaybackBehavior(DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE GetProperties(PXACT_WMA_PLAYLIST_PROPERTIES pProperties);

    //
    // non exported
    //

    DWORD       GetSoundCueIndex(void) const { return m_dwSoundCueIndex; }
    CWmaSong *  GetCurrentSong(void) const   { return m_pCurrent; }

    // Open the current song's decoder, so a cue can stream from it. The
    // playlist keeps ownership; the caller must not Release it.
    HRESULT     OpenCurrentDecoder(XWmaFileMediaObject **ppDecoder);
    VOID        CloseDecoder(void);

private:
    HRESULT     AddFile(LPCSTR pszFileName, PXACTWMASONG *ppSong);
    HRESULT     AddDirectory(LPCSTR pszDirectory, PXACTWMASONG *ppSong);
    HRESULT     AddSoundtrack(DWORD dwSoundtrackId, PXACTWMASONG *ppSong);
    HRESULT     AddSoundtrackSong(DWORD dwSoundtrackId, DWORD dwSongIndex, PXACTWMASONG *ppSong);
    HRESULT     LinkSong(CWmaSong *pSong, PXACTWMASONG *ppSong);
    VOID        Unlink(CWmaSong *pSong);
    CWmaSong *  PickRandom(void) const;
    HRESULT     EnsureDecoder(void);

    CSoundBank *            m_pSoundBank;       // weak: the bank owns us in effect
    DWORD                   m_dwSoundCueIndex;
    DWORD                   m_dwPlaybackFlags;

    CWmaSong *              m_pFirst;
    CWmaSong *              m_pLast;
    CWmaSong *              m_pCurrent;
    DWORD                   m_dwSongCount;

    // Decoder for the current song, opened lazily. GetCurrentSongInfo needs it
    // for the metadata, and the cue needs it to stream -- one decoder serves
    // both, so asking for song info does not disturb playback.
    XWmaFileMediaObject *   m_pDecoder;
    CWmaSong *              m_pDecoderSong;     // which song m_pDecoder is open on

    DWORD                   m_dwRandomSeed;
};

} // namespace XACT

#endif // __WMAPLAYLIST_H__
