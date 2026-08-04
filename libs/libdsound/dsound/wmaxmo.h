/***************************************************************************
 *
 *  RXDK 5849 uplift.
 *
 *  File:       wmaxmo.h
 *  Content:    WMA decoder XMO.
 *
 *  The May-2020 leak ships DirectSound without any WMA support -- the
 *  codecs/ tree it would have lived in is absent -- but 5849 titles get a
 *  WMA decoder as an XMO, so this supplies one over the ported ffmpeg WMA
 *  v1/v2 decoder in wma/.
 *
 *  Two shapes, matching the 5849 factories:
 *
 *    - file-backed  (XWmaDecoderCreateMediaObject, WmaCreateDecoder[Ex]).
 *      Creation does no I/O; the ASF header is read on the first DoWork and
 *      GetStatus withholds XMO_STATUSF_ACCEPT_OUTPUT_DATA until it lands, so
 *      a title can start a stream without blocking on the disk.
 *
 *    - callback-backed  (WmaCreateInMemoryDecoder[Ex]).  The title already
 *      holds the whole file, so the callback hands back pointers into it and
 *      creation parses the header on the spot.
 *
 ***************************************************************************/

#ifndef __WMAXMO_H__
#define __WMAXMO_H__

#ifdef __cplusplus

extern "C"
{
#include "../wma/asf.h"
#include "../wma/wmastream.h"
}

namespace DirectSound
{
    class CWmaMediaObject
        : public XWmaFileMediaObject, public CRefCount
    {
    protected:
        // Data source.  Exactly one of the file handle and the callback is live.
        HANDLE                  m_hFile;                // source file, or INVALID_HANDLE_VALUE
        BOOL                    m_fCloseFile;           // we opened it, so we close it
        DWORD                   m_dwFileBase;           // offset of the ASF stream within the file
        LPFNWMAXMODATACALLBACK  m_pfnCallback;          // in-memory source, or NULL
        LPVOID                  m_pvCallbackContext;

        // Header state.  m_hrHeader latches the parse result so a failure is reported once per
        // call rather than retried on every DoWork.
        AsfFileInfo             m_Asf;
        BOOL                    m_fHeaderParsed;
        HRESULT                 m_hrHeader;
        DWORD                   m_dwLookahead;          // caller's hint at the read size

        WmaStreamDecoder *      m_pDecoder;
        WAVEFORMATEX            m_wfxDecoded;           // PCM format the decoder produces

        // ASF data-packet cursor.
        DWORD                   m_dwPacketIndex;
        BOOL                    m_fEndOfStream;

        // Compressed payloads accumulate here until a whole blockAlign-sized WMA packet is
        // available -- an ASF data packet need not hold a whole number of them.
        LPBYTE                  m_pbCompressed;
        DWORD                   m_cbCompressed;         // capacity
        DWORD                   m_cbCompressedValid;    // bytes held

        // Decoded PCM waiting to be handed to Process().
        LPBYTE                  m_pbPcm;
        DWORD                   m_cbPcm;                // capacity
        DWORD                   m_cbPcmValid;
        DWORD                   m_cbPcmRead;

        // Scratch for one ASF data packet.
        LPBYTE                  m_pbPacket;

    public:
        CWmaMediaObject(void);
        virtual ~CWmaMediaObject(void);

    public:
        // Initialization
        HRESULT STDMETHODCALLTYPE InitializeFile(LPCSTR pszFileName, HANDLE hFile, DWORD dwFileOffset, DWORD dwLookaheadBufferSize);
        HRESULT STDMETHODCALLTYPE InitializeCallback(LPFNWMAXMODATACALLBACK pfnCallback, LPVOID pvContext);

        // The decoded PCM format, for the factories that report it to the caller.
        HRESULT STDMETHODCALLTYPE GetDecodedFormat(LPWAVEFORMATEX pwfx);

        // IUnknown methods
        virtual ULONG STDMETHODCALLTYPE AddRef(void);
        virtual ULONG STDMETHODCALLTYPE Release(void);

        // XMediaObject methods
        virtual HRESULT STDMETHODCALLTYPE GetInfo(LPXMEDIAINFO pInfo);
        virtual HRESULT STDMETHODCALLTYPE GetStatus(LPDWORD pdwStatus);
        virtual HRESULT STDMETHODCALLTYPE Process(LPCXMEDIAPACKET pxmbInput, LPCXMEDIAPACKET pxmbOutput);
        virtual HRESULT STDMETHODCALLTYPE Discontinuity(void);
        virtual HRESULT STDMETHODCALLTYPE Flush(void);

        // XFileMediaObject methods
        virtual HRESULT STDMETHODCALLTYPE Seek(LONG lOffset, DWORD dwOrigin, LPDWORD pdwAbsolute);
        virtual HRESULT STDMETHODCALLTYPE GetLength(LPDWORD pdwLength);
        virtual VOID STDMETHODCALLTYPE DoWork(void);

        // XWmaFileMediaObject methods
        virtual HRESULT STDMETHODCALLTYPE GetFileHeader(WMAXMOFileHeader *pFileHeader);
        virtual HRESULT STDMETHODCALLTYPE GetFileContentDescription(WMAXMOFileContDesc *pContentDesc);
        virtual HRESULT STDMETHODCALLTYPE SeekToTime(DWORD dwSeek, LPDWORD pdwActualSeek);

    protected:
        // Read cbData bytes at dwOffset (relative to the ASF stream) into pbData.  Returns the
        // count actually read, which is short at end of file.
        DWORD ReadAt(DWORD dwOffset, LPBYTE pbData, DWORD cbData);

        HRESULT EnsureHeader(void);
        HRESULT DecodeMorePcm(void);        // advance one ASF packet's worth
        DWORD   TotalPcmBytes(void) const;
        void    ResetPosition(DWORD dwPacketIndex);
    };
}

#endif // __cplusplus

#endif // __WMAXMO_H__
