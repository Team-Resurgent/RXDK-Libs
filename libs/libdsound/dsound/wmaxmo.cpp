/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * WMA decoder XMO. Wraps the ported WMA v1/v2 decoder in wma/ as a media
 * object, in both file-backed and in-memory shapes, so a title can play .wma
 * streams.
 */

#include "dsoundi.h"
#include "wmaxmo.h"

//
//  Enough of the ASF header to find its length, then to parse it.  Real .wma headers are a few
//  kilobytes; the cap keeps a corrupt length field from asking for an absurd allocation.
//

#define WMAXMO_HEADER_PREFIX_BYTES  30
#define WMAXMO_MAX_HEADER_BYTES     (256 * 1024)

//
//  Staging window for a handle whose reads have to be sector-aligned, and the sector size to
//  assume when the volume will not say.  The DVD's is the coarser of the two media, and a
//  request aligned for the DVD is also aligned for the hard disk.
//
#define WMAXMO_STAGING_BYTES        (64 * 1024)
#define WMAXMO_DEFAULT_SECTOR_BYTES 2048

//
//  Most a title's data callback is ever asked for in one call.  The XDK's codec pulled the ASF
//  stream through the callback in small zero-copy pieces, so titles were written to serve one
//  modest request at a time: a callback backed by a double buffer only has to patch a read that
//  straddles the seam, and the shipped samples size that patch buffer at exactly 128 bytes and
//  assert on anything larger.  Reads here are satisfied a chunk at a time to stay inside that.
//
#define WMAXMO_CALLBACK_CHUNK_BYTES 128


/****************************************************************************
 *
 *  CWmaMediaObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::CWmaMediaObject"

CWmaMediaObject::CWmaMediaObject
(
    void
)
{
    DPF_ENTER();

    m_hFile = INVALID_HANDLE_VALUE;
    m_fCloseFile = FALSE;
    m_dwFileBase = 0;
    m_pfnCallback = NULL;
    m_pvCallbackContext = NULL;

    ZeroMemory(&m_Asf, sizeof(m_Asf));
    m_fHeaderParsed = FALSE;
    m_hrHeader = S_OK;
    m_dwLookahead = 0;

    m_pDecoder = NULL;
    ZeroMemory(&m_wfxDecoded, sizeof(m_wfxDecoded));

    m_dwPacketIndex = 0;
    m_fEndOfStream = FALSE;

    m_pbCompressed = NULL;
    m_cbCompressed = 0;
    m_cbCompressedValid = 0;

    m_pbPcm = NULL;
    m_cbPcm = 0;
    m_cbPcmValid = 0;
    m_cbPcmRead = 0;

    m_pbPacket = NULL;

    m_dwSectorSize = 0;
    m_hOverlappedEvent = NULL;
    m_pbStaging = NULL;
    m_cbStaging = 0;

    DPF_LEAVE_VOID();
}


/****************************************************************************
 *
 *  ~CWmaMediaObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::~CWmaMediaObject"

CWmaMediaObject::~CWmaMediaObject
(
    void
)
{
    DPF_ENTER();

    if(m_pDecoder)
    {
        WmaStreamClose(m_pDecoder);
        m_pDecoder = NULL;
    }

    MEMFREE(m_pbCompressed);
    MEMFREE(m_pbPcm);
    MEMFREE(m_pbPacket);

    if(m_pbStaging)
    {
        VirtualFree(m_pbStaging, 0, MEM_RELEASE);
        m_pbStaging = NULL;
    }

    if(m_hOverlappedEvent)
    {
        CloseHandle(m_hOverlappedEvent);
        m_hOverlappedEvent = NULL;
    }

    if(m_fCloseFile && INVALID_HANDLE_VALUE != m_hFile)
    {
        CloseHandle(m_hFile);
    }

    m_hFile = INVALID_HANDLE_VALUE;

    DPF_LEAVE_VOID();
}


/****************************************************************************
 *
 *  AddRef / Release
 *
 ****************************************************************************/

ULONG
CWmaMediaObject::AddRef
(
    void
)
{
    return CRefCount::AddRef();
}

ULONG
CWmaMediaObject::Release
(
    void
)
{
    return CRefCount::Release();
}


/****************************************************************************
 *
 *  InitializeFile
 *
 *  Description:
 *      Attaches the object to a WMA file.  Deliberately does no I/O: the
 *      header is read on the first DoWork, so a title can create the decoder
 *      on its render thread without stalling.
 *
 *  Arguments:
 *      LPCSTR [in]: file name, or NULL when a handle is supplied.
 *      HANDLE [in]: open file handle, or INVALID_HANDLE_VALUE.
 *      DWORD [in]: offset of the ASF stream within the file.
 *      DWORD [in]: caller's read-size hint.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::InitializeFile"

HRESULT
CWmaMediaObject::InitializeFile
(
    LPCSTR                  pszFileName,
    HANDLE                  hFile,
    DWORD                   dwFileOffset,
    DWORD                   dwLookaheadBufferSize
)
{
    HRESULT                 hr = DS_OK;

    DPF_ENTER();

    m_dwFileBase = dwFileOffset;
    m_dwLookahead = dwLookaheadBufferSize;

    if(INVALID_HANDLE_VALUE != hFile && NULL != hFile)
    {
        m_hFile = hFile;
        m_fCloseFile = FALSE;
    }
    else if(pszFileName)
    {
        m_hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(INVALID_HANDLE_VALUE == m_hFile)
        {
            DPF_ERROR("Unable to open the WMA file");
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            m_fCloseFile = TRUE;
        }
    }
    else
    {
        DPF_ERROR("Neither a file name nor a file handle was supplied");
        hr = E_INVALIDARG;
    }

    if(SUCCEEDED(hr))
    {
        BindHandle();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  BindHandle
 *
 *  Description:
 *      Notes what the handle we have just taken on will accept, so ReadAt can
 *      pick a read path that suits it.
 *
 *      A title reaches this object with a handle of its own
 *      choosing, and the one an XACT streamed wave bank arrives on is opened
 *      FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING.  Neither flag tolerates
 *      the plain seek-and-read below: an asynchronous handle has no file
 *      position to seek, and refuses a read that does not name its own offset;
 *      an unbuffered one refuses a read that is not a whole number of sectors
 *      beginning on a sector boundary, which the ASF header prefix and the
 *      data packets never are.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::BindHandle"

void
CWmaMediaObject::BindHandle
(
    void
)
{
    IO_STATUS_BLOCK          iosb;
    FILE_MODE_INFORMATION    fmi;
    FILE_FS_SIZE_INFORMATION fsSize;
    BOOL                     fAsync;
    BOOL                     fUnbuffered;

    DPF_ENTER();

    m_dwSectorSize = 0;

    if(INVALID_HANDLE_VALUE != m_hFile &&
       NT_SUCCESS(NtQueryInformationFile(m_hFile, &iosb, &fmi, sizeof(fmi), FileModeInformation)))
    {
        fAsync = !(fmi.Mode & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT));
        fUnbuffered = !!(fmi.Mode & FILE_NO_INTERMEDIATE_BUFFERING);

        if(fUnbuffered)
        {
            if(NT_SUCCESS(NtQueryVolumeInformationFile(m_hFile, &iosb, &fsSize, sizeof(fsSize), FileFsSizeInformation)) &&
               fsSize.BytesPerSector)
            {
                m_dwSectorSize = fsSize.BytesPerSector;
            }
            else
            {
                m_dwSectorSize = WMAXMO_DEFAULT_SECTOR_BYTES;
            }
        }
        else if(fAsync)
        {
            //
            // Reads must name their offset, but need no rounding.
            //

            m_dwSectorSize = 1;
        }
    }

    DPF_LEAVE_VOID();
}


/****************************************************************************
 *
 *  ReadFileRaw
 *
 *  Description:
 *      Reads at an explicit file offset, waiting for the transfer to finish.
 *      Works whether or not the handle is asynchronous, which is what lets one
 *      path serve both.
 *
 *  Arguments:
 *      DWORD [in]: absolute file offset.
 *      LPVOID [out]: destination.
 *      DWORD [in]: byte count.
 *      LPDWORD [out]: bytes read.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::ReadFileRaw"

HRESULT
CWmaMediaObject::ReadFileRaw
(
    DWORD                   dwFileOffset,
    LPVOID                  pvBuffer,
    DWORD                   cbBuffer,
    LPDWORD                 pcbRead
)
{
    HRESULT                 hr      = S_OK;
    OVERLAPPED              ov      = {0};
    DWORD                   cbRead  = 0;

    DPF_ENTER();

    if(!m_hOverlappedEvent)
    {
        m_hOverlappedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        if(!m_hOverlappedEvent)
        {
            DPF_ERROR("Error %lu occurred creating the read event", GetLastError());
            hr = E_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(hr))
    {
        ov.Offset = dwFileOffset;
        ov.hEvent = m_hOverlappedEvent;

        ResetEvent(m_hOverlappedEvent);

        if(!ReadFile(m_hFile, pvBuffer, cbBuffer, &cbRead, &ov))
        {
            switch(GetLastError())
            {
                case ERROR_IO_PENDING:
                    if(!GetOverlappedResult(m_hFile, &ov, &cbRead, TRUE) &&
                       ERROR_HANDLE_EOF != GetLastError())
                    {
                        DPF_ERROR("Error %lu occurred completing the read", GetLastError());
                        hr = E_FAIL;
                    }
                    break;

                //
                // Reading at the end of the file is how the ASF parser discovers where the
                // stream ends, so it is a short read rather than a failure.
                //

                case ERROR_HANDLE_EOF:
                    cbRead = 0;
                    break;

                default:
                    DPF_ERROR("Error %lu occurred reading the WMA stream", GetLastError());
                    hr = E_FAIL;
                    break;
            }
        }
    }

    *pcbRead = SUCCEEDED(hr) ? cbRead : 0;

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  ReadFileStaged
 *
 *  Description:
 *      Reads an arbitrary window of an unbuffered handle by reading the whole
 *      sectors that cover it into an aligned buffer and copying the window
 *      out.  Loops when the window is larger than that buffer.
 *
 *  Arguments:
 *      DWORD [in]: absolute file offset.
 *      LPBYTE [out]: destination.
 *      DWORD [in]: byte count.
 *
 *  Returns:
 *      DWORD: bytes read (short at end of file).
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::ReadFileStaged"

DWORD
CWmaMediaObject::ReadFileStaged
(
    DWORD                   dwFileOffset,
    LPBYTE                  pbData,
    DWORD                   cbData
)
{
    DWORD                   cbTotal = 0;
    DWORD                   dwAligned;
    DWORD                   dwSkip;

    DPF_ENTER();

    if(!m_pbStaging)
    {
        //
        // A virtual allocation is page-aligned, hence aligned for either medium's sectors.
        //

        m_pbStaging = (LPBYTE)VirtualAlloc(NULL, WMAXMO_STAGING_BYTES, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if(!m_pbStaging)
        {
            DPF_ERROR("Unable to allocate the staging buffer");
            DPF_LEAVE(0);
            return 0;
        }

        m_cbStaging = WMAXMO_STAGING_BYTES;
    }

    dwAligned = dwFileOffset - (dwFileOffset % m_dwSectorSize);
    dwSkip = dwFileOffset - dwAligned;

    while(cbTotal < cbData)
    {
        DWORD               cbWanted = dwSkip + (cbData - cbTotal);
        DWORD               cbChunk  = (cbWanted + m_dwSectorSize - 1) / m_dwSectorSize * m_dwSectorSize;
        DWORD               cbRead   = 0;
        DWORD               cbCopy;

        if(cbChunk > m_cbStaging)
        {
            cbChunk = m_cbStaging;
        }

        if(FAILED(ReadFileRaw(dwAligned, m_pbStaging, cbChunk, &cbRead)) || cbRead <= dwSkip)
        {
            break;
        }

        cbCopy = cbRead - dwSkip;

        if(cbCopy > cbData - cbTotal)
        {
            cbCopy = cbData - cbTotal;
        }

        CopyMemory(pbData + cbTotal, m_pbStaging + dwSkip, cbCopy);
        cbTotal += cbCopy;

        if(cbRead < cbChunk)
        {
            //
            // Short of what whole sectors would have given us: end of file.
            //

            break;
        }

        dwAligned += cbChunk;
        dwSkip = 0;
    }

    DPF_LEAVE(cbTotal);

    return cbTotal;
}


/****************************************************************************
 *
 *  ReadFileAt
 *
 *  Description:
 *      Reads from the file handle by whichever path it will accept.
 *
 *  Arguments:
 *      DWORD [in]: absolute file offset.
 *      LPBYTE [out]: destination.
 *      DWORD [in]: byte count.
 *
 *  Returns:
 *      DWORD: bytes read (short at end of file).
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::ReadFileAt"

DWORD
CWmaMediaObject::ReadFileAt
(
    DWORD                   dwFileOffset,
    LPBYTE                  pbData,
    DWORD                   cbData
)
{
    DWORD                   cbRead = 0;

    if(m_dwSectorSize > 1)
    {
        cbRead = ReadFileStaged(dwFileOffset, pbData, cbData);
    }
    else if(m_dwSectorSize)
    {
        if(FAILED(ReadFileRaw(dwFileOffset, pbData, cbData, &cbRead)))
        {
            cbRead = 0;
        }
    }
    else if(INVALID_SET_FILE_POINTER != SetFilePointer(m_hFile, (LONG)dwFileOffset, NULL, FILE_BEGIN))
    {
        if(!ReadFile(m_hFile, pbData, cbData, &cbRead, NULL))
        {
            cbRead = 0;
        }
    }

    return cbRead;
}


/****************************************************************************
 *
 *  InitializeCallback
 *
 *  Description:
 *      Attaches the object to a title-supplied in-memory data source.  The
 *      title already holds the file, so there is nothing to wait for and the
 *      header is parsed immediately.
 *
 *  Arguments:
 *      LPFNWMAXMODATACALLBACK [in]: data callback.
 *      LPVOID [in]: callback context.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::InitializeCallback"

HRESULT
CWmaMediaObject::InitializeCallback
(
    LPFNWMAXMODATACALLBACK  pfnCallback,
    LPVOID                  pvContext
)
{
    HRESULT                 hr;

    DPF_ENTER();

    if(!pfnCallback)
    {
        DPF_ERROR("No data callback supplied");
        hr = E_INVALIDARG;
    }
    else
    {
        m_pfnCallback = pfnCallback;
        m_pvCallbackContext = pvContext;

        hr = EnsureHeader();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  ReadAt
 *
 *  Description:
 *      Reads from whichever data source the object was initialized with.
 *
 *  Arguments:
 *      DWORD [in]: offset within the ASF stream.
 *      LPBYTE [out]: destination.
 *      DWORD [in]: byte count.
 *
 *  Returns:
 *      DWORD: bytes actually read (short at end of file).
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::ReadAt"

DWORD
CWmaMediaObject::ReadAt
(
    DWORD                   dwOffset,
    LPBYTE                  pbData,
    DWORD                   cbData
)
{
    DWORD                   cbRead = 0;

    if(m_pfnCallback)
    {
        while(cbRead < cbData)
        {
            LPVOID          pvData = NULL;
            DWORD           cbChunk = cbData - cbRead;
            DWORD           cbAvailable;

            if(cbChunk > WMAXMO_CALLBACK_CHUNK_BYTES)
            {
                cbChunk = WMAXMO_CALLBACK_CHUNK_BYTES;
            }

            cbAvailable = m_pfnCallback(m_pvCallbackContext, dwOffset + cbRead, cbChunk, &pvData);

            if(!pvData || !cbAvailable)
            {
                break;
            }

            if(cbAvailable > cbChunk)
            {
                cbAvailable = cbChunk;
            }

            CopyMemory(pbData + cbRead, pvData, cbAvailable);
            cbRead += cbAvailable;
        }
    }
    else if(INVALID_HANDLE_VALUE != m_hFile)
    {
        cbRead = ReadFileAt(m_dwFileBase + dwOffset, pbData, cbData);
    }

    return cbRead;
}


/****************************************************************************
 *
 *  EnsureHeader
 *
 *  Description:
 *      Reads and parses the ASF header, then stands up the decoder.  The
 *      result is latched: a malformed file fails once rather than being
 *      re-read on every DoWork.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::EnsureHeader"

HRESULT
CWmaMediaObject::EnsureHeader
(
    void
)
{
    BYTE                    abPrefix[WMAXMO_HEADER_PREFIX_BYTES];
    LPBYTE                  pbHeader = NULL;
    unsigned int            cbHeader;
    DWORD                   cbTotal;
    HRESULT                 hr = DS_OK;

    DPF_ENTER();

    if(m_fHeaderParsed)
    {
        DPF_LEAVE_HRESULT(m_hrHeader);
        return m_hrHeader;
    }

    //
    // The Header Object carries its own size, so read just enough to learn it before committing
    // to an allocation.
    //

    if(WMAXMO_HEADER_PREFIX_BYTES != ReadAt(0, abPrefix, WMAXMO_HEADER_PREFIX_BYTES))
    {
        DPF_ERROR("Unable to read the ASF header prefix");
        hr = E_FAIL;
    }

    if(SUCCEEDED(hr))
    {
        if(AsfPeekHeaderSize(abPrefix, WMAXMO_HEADER_PREFIX_BYTES, &cbHeader) < 0)
        {
            DPF_ERROR("Not an ASF file");
            hr = E_INVALIDARG;
        }
        else if(cbHeader > WMAXMO_MAX_HEADER_BYTES)
        {
            DPF_ERROR("ASF header is implausibly large");
            hr = E_INVALIDARG;
        }
    }

    if(SUCCEEDED(hr))
    {
        //
        // AsfParseHeader also wants the Data Object header that follows, so that it can report
        // where the packets start.
        //

        cbTotal = (DWORD)cbHeader + 64;

        hr = HRFROMP(pbHeader = MEMALLOC_NOINIT(BYTE, cbTotal));
    }

    if(SUCCEEDED(hr))
    {
        DWORD cbRead = ReadAt(0, pbHeader, cbTotal);

        if(AsfParseHeader(pbHeader, cbRead, &m_Asf) < 0)
        {
            DPF_ERROR("Unable to parse the ASF header");
            hr = E_FAIL;
        }
    }

    //
    // Stand up the decoder and the buffers sized from what the header said.
    //

    if(SUCCEEDED(hr))
    {
        if(WmaStreamOpen(m_Asf.formatTag, m_Asf.channels, m_Asf.sampleRate,
                         m_Asf.avgBytesPerSec * 8, m_Asf.blockAlign,
                         m_Asf.extradata, m_Asf.extradataSize, &m_pDecoder) < 0)
        {
            DPF_ERROR("Unable to create the WMA decoder");
            hr = E_FAIL;
        }
    }

    if(SUCCEEDED(hr))
    {
        XAudioCreatePcmFormat(m_Asf.channels, m_Asf.sampleRate, 16, &m_wfxDecoded);

        //
        // One ASF data packet can carry more than one compressed WMA packet, and can end
        // mid-packet, so the accumulator holds a whole ASF packet plus the partial WMA packet
        // carried over from the previous one.
        //

        m_cbCompressed = m_Asf.packetSize + m_Asf.blockAlign;
        m_cbPcm = (DWORD)WmaStreamMaxOutputBytes(m_pDecoder);

        hr = HRFROMP(m_pbPacket = MEMALLOC_NOINIT(BYTE, m_Asf.packetSize));

        if(SUCCEEDED(hr))
        {
            hr = HRFROMP(m_pbCompressed = MEMALLOC_NOINIT(BYTE, m_cbCompressed));
        }

        if(SUCCEEDED(hr))
        {
            hr = HRFROMP(m_pbPcm = MEMALLOC_NOINIT(BYTE, m_cbPcm));
        }
    }

    m_fHeaderParsed = TRUE;
    m_hrHeader = hr;

    MEMFREE(pbHeader);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  DecodeMorePcm
 *
 *  Description:
 *      Pulls the next ASF data packet, hands its payloads to the decoder and
 *      refills the PCM buffer.  Sets m_fEndOfStream once the data object is
 *      exhausted.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: COM result code.  S_FALSE means no PCM was produced.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::DecodeMorePcm"

HRESULT
CWmaMediaObject::DecodeMorePcm
(
    void
)
{
    DWORD                   cbRead;
    unsigned int            cbPayload = 0;
    DWORD                   cbDecoded = 0;
    HRESULT                 hr = DS_OK;

    DPF_ENTER();

    m_cbPcmValid = 0;
    m_cbPcmRead = 0;

    //
    // Pull ASF packets until the accumulator holds at least one whole compressed WMA packet.
    //

    while(m_cbCompressedValid < m_Asf.blockAlign)
    {
        if(m_Asf.dataPacketCount && m_dwPacketIndex >= m_Asf.dataPacketCount)
        {
            m_fEndOfStream = TRUE;
            break;
        }

        cbRead = ReadAt(m_Asf.dataOffset + m_dwPacketIndex * m_Asf.packetSize,
                        m_pbPacket, m_Asf.packetSize);

        if(cbRead < m_Asf.packetSize)
        {
            m_fEndOfStream = TRUE;
            break;
        }

        m_dwPacketIndex++;

        if(AsfParsePacket(&m_Asf, m_pbPacket, cbRead,
                          m_pbCompressed + m_cbCompressedValid,
                          m_cbCompressed - m_cbCompressedValid, &cbPayload) < 0)
        {
            //
            // A packet we can't parse is not fatal -- skip it rather than end the stream, which
            // would truncate playback over a single bad packet.
            //

            continue;
        }

        m_cbCompressedValid += cbPayload;
    }

    if(m_cbCompressedValid < m_Asf.blockAlign)
    {
        //
        // End of stream with a partial packet left over: there is nothing more to decode.
        //

        m_cbCompressedValid = 0;
        hr = S_FALSE;
    }
    else
    {
        int                 cbOut = 0;

        if(WmaStreamDecode(m_pDecoder, m_pbCompressed, (int)m_Asf.blockAlign,
                           (short *)m_pbPcm, (int)m_cbPcm, &cbOut) < 0)
        {
            //
            // A packet the decoder rejects costs us that packet, not the stream.
            //

            cbOut = 0;
        }

        cbDecoded = (DWORD)cbOut;

        //
        // Retire the packet we just consumed and shuffle the remainder down.
        //

        m_cbCompressedValid -= m_Asf.blockAlign;

        if(m_cbCompressedValid)
        {
            MoveMemory(m_pbCompressed, m_pbCompressed + m_Asf.blockAlign, m_cbCompressedValid);
        }

        m_cbPcmValid = cbDecoded;

        //
        // The first packet of a bit-reservoir stream decodes to nothing; that is not end of
        // stream, so tell the caller to come back rather than treating it as EOF.
        //

        if(0 == cbDecoded)
        {
            hr = S_FALSE;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  TotalPcmBytes
 *
 *  Description:
 *      Total decoded size of the stream, from the header's duration.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      DWORD: byte count.
 *
 ****************************************************************************/

DWORD
CWmaMediaObject::TotalPcmBytes
(
    void
) const
{
    if(!m_fHeaderParsed || FAILED(m_hrHeader))
    {
        return 0;
    }

    //
    // Compute in samples rather than bytes: duration * sample rate overflows 32 bits for any
    // stream longer than about 27 hours at 44.1kHz, but only after the divide by 1000.
    //

    return (m_Asf.durationMs / 1000) * m_wfxDecoded.nAvgBytesPerSec +
           ((m_Asf.durationMs % 1000) * m_wfxDecoded.nAvgBytesPerSec) / 1000;
}


/****************************************************************************
 *
 *  ResetPosition
 *
 *  Description:
 *      Repositions to an ASF packet index and drops all decoder state.  WMA
 *      carries a bit reservoir and overlap-add history across packets, so
 *      decoding from a new position without this produces garbage.
 *
 *  Arguments:
 *      DWORD [in]: ASF data-packet index.
 *
 *  Returns:
 *      (void)
 *
 ****************************************************************************/

void
CWmaMediaObject::ResetPosition
(
    DWORD                   dwPacketIndex
)
{
    m_dwPacketIndex = dwPacketIndex;
    m_fEndOfStream = FALSE;
    m_cbCompressedValid = 0;
    m_cbPcmValid = 0;
    m_cbPcmRead = 0;

    if(m_pDecoder)
    {
        WmaStreamReset(m_pDecoder);
    }
}


/****************************************************************************
 *
 *  GetInfo
 *
 *  Description:
 *      Retrieves the object's stream characteristics.
 *
 *  Arguments:
 *      LPXMEDIAINFO [out]: receives the information.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::GetInfo"

HRESULT
CWmaMediaObject::GetInfo
(
    LPXMEDIAINFO            pInfo
)
{
    DPF_ENTER();

    pInfo->dwFlags = XMO_STREAMF_FIXED_SAMPLE_SIZE;
    pInfo->dwMaxLookahead = 0;
    pInfo->dwInputSize = 0;

    //
    // Output granularity is one PCM sample frame.  Before the header lands we don't know the
    // channel count, so report the mono figure -- titles that check this call it after waiting
    // for XMO_STATUSF_ACCEPT_OUTPUT_DATA.
    //

    pInfo->dwOutputSize = m_wfxDecoded.nBlockAlign ? m_wfxDecoded.nBlockAlign : 2;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/****************************************************************************
 *
 *  GetStatus
 *
 *  Description:
 *      Retrieves the object's status.  Output is withheld until the header
 *      has been read; that is the signal file-backed titles poll on.
 *
 *  Arguments:
 *      LPDWORD [out]: receives the status.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::GetStatus"

HRESULT
CWmaMediaObject::GetStatus
(
    LPDWORD                 pdwStatus
)
{
    DPF_ENTER();

    *pdwStatus = (m_fHeaderParsed && SUCCEEDED(m_hrHeader)) ? XMO_STATUSF_ACCEPT_OUTPUT_DATA : 0;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/****************************************************************************
 *
 *  Process
 *
 *  Description:
 *      Produces decoded PCM.  Output-only: pxmbInput is ignored.  A short
 *      completed size means the stream ended.
 *
 *  Arguments:
 *      LPCXMEDIAPACKET [in]: unused.
 *      LPCXMEDIAPACKET [in]: destination packet.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::Process"

HRESULT
CWmaMediaObject::Process
(
    LPCXMEDIAPACKET         pxmbSource,
    LPCXMEDIAPACKET         pxmbDest
)
{
    LPBYTE                  pbDst;
    DWORD                   cbDst;
    DWORD                   cbWritten = 0;
    HRESULT                 hr;

    DPF_ENTER();

    UNREFERENCED_PARAMETER(pxmbSource);

    ASSERT(pxmbDest);
    ASSERT(pxmbDest->pvBuffer);

    hr = EnsureHeader();

    if(FAILED(hr))
    {
        XMOAcceptPacket(pxmbDest);
        XMOCompletePacket(pxmbDest, 0, NULL, NULL, XMEDIAPACKET_STATUS_FAILURE);

        DPF_LEAVE_HRESULT(hr);
        return hr;
    }

    pbDst = (LPBYTE)pxmbDest->pvBuffer;
    cbDst = pxmbDest->dwMaxSize;

    //
    // Sample-align the size -- we promised XMO_STREAMF_FIXED_SAMPLE_SIZE.
    //

    cbDst -= cbDst % m_wfxDecoded.nBlockAlign;

    XMOAcceptPacket(pxmbDest);

    while(cbWritten < cbDst)
    {
        DWORD               cbAvailable = m_cbPcmValid - m_cbPcmRead;
        DWORD               cbCopy;

        if(!cbAvailable)
        {
            if(m_fEndOfStream)
            {
                break;
            }

            //
            // S_FALSE means this packet produced nothing; keep going unless that was because the
            // stream ran out.
            //

            if(FAILED(DecodeMorePcm()))
            {
                break;
            }

            continue;
        }

        cbCopy = cbDst - cbWritten;

        if(cbCopy > cbAvailable)
        {
            cbCopy = cbAvailable;
        }

        CopyMemory(pbDst + cbWritten, m_pbPcm + m_cbPcmRead, cbCopy);

        m_cbPcmRead += cbCopy;
        cbWritten += cbCopy;
    }

    XMOCompletePacket(pxmbDest, cbWritten);

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/****************************************************************************
 *
 *  Discontinuity
 *
 ****************************************************************************/

HRESULT
CWmaMediaObject::Discontinuity
(
    void
)
{
    return DS_OK;
}


/****************************************************************************
 *
 *  Flush
 *
 *  Description:
 *      Rewinds to the start of the stream.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::Flush"

HRESULT
CWmaMediaObject::Flush
(
    void
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = EnsureHeader();

    if(SUCCEEDED(hr))
    {
        ResetPosition(0);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  Seek
 *
 *  Description:
 *      Repositions the stream.  Offsets are in decoded PCM bytes, matching
 *      the wave file XMO.
 *
 *  Arguments:
 *      LONG [in]: offset.
 *      DWORD [in]: FILE_BEGIN / FILE_CURRENT / FILE_END.
 *      LPDWORD [out]: receives the resulting absolute offset.  May be NULL.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::Seek"

HRESULT
CWmaMediaObject::Seek
(
    LONG                    lOffset,
    DWORD                   dwOrigin,
    LPDWORD                 pdwAbsolute
)
{
    DWORD                   dwTotal;
    LONG                    lTarget;
    HRESULT                 hr;

    DPF_ENTER();

    hr = EnsureHeader();

    if(SUCCEEDED(hr))
    {
        dwTotal = TotalPcmBytes();

        switch(dwOrigin)
        {
            case FILE_BEGIN:
                lTarget = lOffset;
                break;

            case FILE_CURRENT:
                //
                // Current position is where the packet cursor sits, less whatever of the decoded
                // packet the caller has not taken yet.
                //

                lTarget = (LONG)(((DWORD)((m_dwPacketIndex * (unsigned __int64)dwTotal) /
                                          (m_Asf.dataPacketCount ? m_Asf.dataPacketCount : 1))) +
                                 m_cbPcmRead) + lOffset;
                break;

            case FILE_END:
                lTarget = (LONG)dwTotal + lOffset;
                break;

            default:
                DPF_ERROR("Invalid seek origin");
                hr = E_INVALIDARG;
                lTarget = 0;
                break;
        }
    }

    if(SUCCEEDED(hr))
    {
        DWORD               dwTarget;
        DWORD               dwPacket = 0;

        dwTarget = (lTarget < 0) ? 0 : (DWORD)lTarget;

        if(dwTarget > dwTotal)
        {
            dwTarget = dwTotal;
        }

        //
        // WMA has no per-sample index, so map the PCM offset onto a data packet by proportion.
        // The result lands on a packet boundary, which is the finest granularity a seek can have
        // without decoding forward from the start.
        //

        if(dwTotal && m_Asf.dataPacketCount)
        {
            dwPacket = (DWORD)(((unsigned __int64)dwTarget * m_Asf.dataPacketCount) / dwTotal);

            if(dwPacket > m_Asf.dataPacketCount)
            {
                dwPacket = m_Asf.dataPacketCount;
            }
        }

        ResetPosition(dwPacket);

        if(pdwAbsolute)
        {
            *pdwAbsolute = dwTarget;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  GetLength
 *
 *  Description:
 *      Retrieves the total decoded size of the stream.
 *
 *  Arguments:
 *      LPDWORD [out]: receives the length in bytes.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::GetLength"

HRESULT
CWmaMediaObject::GetLength
(
    LPDWORD                 pdwLength
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = EnsureHeader();

    if(SUCCEEDED(hr))
    {
        *pdwLength = TotalPcmBytes();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  DoWork
 *
 *  Description:
 *      Performs deferred work.  For a file-backed decoder that is the header
 *      read; the title polls GetStatus until it completes.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::DoWork"

VOID
CWmaMediaObject::DoWork
(
    void
)
{
    DPF_ENTER();

    (void)EnsureHeader();

    DPF_LEAVE_VOID();
}


/****************************************************************************
 *
 *  GetFileHeader
 *
 *  Description:
 *      Retrieves the WMA stream's format summary.
 *
 *  Arguments:
 *      WMAXMOFileHeader * [out]: receives the header.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::GetFileHeader"

HRESULT
CWmaMediaObject::GetFileHeader
(
    WMAXMOFileHeader *      pFileHeader
)
{
    HRESULT                 hr;

    DPF_ENTER();

#ifdef VALIDATE_PARAMETERS

    if(!pFileHeader)
    {
        DPF_ERROR("No header pointer supplied");
    }

#endif // VALIDATE_PARAMETERS

    hr = EnsureHeader();

    if(SUCCEEDED(hr))
    {
        pFileHeader->dwVersion = (0x0160 == m_Asf.formatTag) ? 1 : 2;
        pFileHeader->dwSampleRate = m_Asf.sampleRate;
        pFileHeader->dwNumChannels = m_Asf.channels;
        pFileHeader->dwDuration = m_Asf.durationMs;
        pFileHeader->dwBitrate = m_Asf.avgBytesPerSec * 8;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  GetFileContentDescription
 *
 *  Description:
 *      Retrieves the file's metadata strings.  The caller supplies both the
 *      buffers and their sizes (in bytes); each field is optional.
 *
 *  Arguments:
 *      WMAXMOFileContDesc * [in/out]: buffers in, strings out.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::GetFileContentDescription"

//
// The caller's lengths are byte counts, so convert, and always leave room for the terminator.
//

static void CopyContentString(WCHAR *pDest, WORD wDestBytes, const unsigned short *pSrc)
{
    DWORD                   cchDest;
    DWORD                   i;

    if(!pDest || wDestBytes < sizeof(WCHAR))
    {
        return;
    }

    cchDest = wDestBytes / sizeof(WCHAR);

    for(i = 0; i < cchDest - 1 && pSrc[i]; i++)
    {
        pDest[i] = (WCHAR)pSrc[i];
    }

    pDest[i] = 0;
}

HRESULT
CWmaMediaObject::GetFileContentDescription
(
    WMAXMOFileContDesc *    pContentDesc
)
{
    HRESULT                 hr;

    DPF_ENTER();

#ifdef VALIDATE_PARAMETERS

    if(!pContentDesc)
    {
        DPF_ERROR("No content description pointer supplied");
    }

#endif // VALIDATE_PARAMETERS

    hr = EnsureHeader();

    if(SUCCEEDED(hr))
    {
        CopyContentString(pContentDesc->pTitle, pContentDesc->wTitleLength, m_Asf.title);
        CopyContentString(pContentDesc->pAuthor, pContentDesc->wAuthorLength, m_Asf.author);
        CopyContentString(pContentDesc->pCopyright, pContentDesc->wCopyrightLength, m_Asf.copyright);
        CopyContentString(pContentDesc->pDescription, pContentDesc->wDescriptionLength, m_Asf.description);
        CopyContentString(pContentDesc->pRating, pContentDesc->wRatingLength, m_Asf.rating);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  SeekToTime
 *
 *  Description:
 *      Repositions the stream to a time offset, in milliseconds.
 *
 *  Arguments:
 *      DWORD [in]: target time, in milliseconds.
 *      LPDWORD [out]: receives the time actually seeked to.  May be NULL.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::SeekToTime"

HRESULT
CWmaMediaObject::SeekToTime
(
    DWORD                   dwSeek,
    LPDWORD                 pdwActualSeek
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = EnsureHeader();

    if(SUCCEEDED(hr))
    {
        DWORD               dwPacket = 0;

        if(dwSeek > m_Asf.durationMs)
        {
            dwSeek = m_Asf.durationMs;
        }

        if(m_Asf.durationMs && m_Asf.dataPacketCount)
        {
            dwPacket = (DWORD)(((unsigned __int64)dwSeek * m_Asf.dataPacketCount) / m_Asf.durationMs);
        }

        ResetPosition(dwPacket);

        if(pdwActualSeek)
        {
            //
            // Seeks land on packet boundaries, so report where we actually ended up rather than
            // what was asked for.
            //

            *pdwActualSeek = m_Asf.dataPacketCount
                           ? (DWORD)(((unsigned __int64)dwPacket * m_Asf.durationMs) / m_Asf.dataPacketCount)
                           : 0;
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  GetDecodedFormat
 *
 *  Description:
 *      Retrieves the PCM format the decoder produces.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [out]: receives the format.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWmaMediaObject::GetDecodedFormat"

HRESULT
CWmaMediaObject::GetDecodedFormat
(
    LPWAVEFORMATEX          pwfx
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = EnsureHeader();

    if(SUCCEEDED(hr))
    {
        CopyMemory(pwfx, &m_wfxDecoded, sizeof(WAVEFORMATEX));
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  XWmaDecoderCreateMediaObject
 *
 *  Description:
 *      Creates a WMA decoder over a file, without blocking on I/O.  The
 *      header is read on the first DoWork; until then GetStatus withholds
 *      XMO_STATUSF_ACCEPT_OUTPUT_DATA.
 *
 *  Arguments:
 *      LPCWMAXMODECODERPARAMETERS [in]: source description.
 *      XWmaFileMediaObject ** [out]: receives the object.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "XWmaDecoderCreateMediaObject"

HRESULT
XWmaDecoderCreateMediaObject
(
    LPCWMAXMODECODERPARAMETERS  pParameters,
    XWmaFileMediaObject **      ppMediaObject
)
{
    CWmaMediaObject *       pMediaObject;
    HRESULT                 hr;

    DPF_ENTER();

#ifdef VALIDATE_PARAMETERS

    if(!pParameters)
    {
        DPF_ERROR("No parameters supplied");
    }

    if(!ppMediaObject)
    {
        DPF_ERROR("No media object pointer supplied");
    }

#endif // VALIDATE_PARAMETERS

    hr = HRFROMP(pMediaObject = NEW(CWmaMediaObject));

    if(SUCCEEDED(hr))
    {
        hr = pMediaObject->InitializeFile(pParameters->pszFileName,
                                          pParameters->hFile,
                                          pParameters->dwFileOffset,
                                          pParameters->dwLookaheadBufferSize);
    }

    if(SUCCEEDED(hr))
    {
        *ppMediaObject = ADDREF(pMediaObject);
    }

    RELEASE(pMediaObject);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  WmaCreateDecoderEx
 *
 *  Description:
 *      Creates a WMA decoder over a file and reports the PCM format it will
 *      produce.  Unlike XWmaDecoderCreateMediaObject this reads the header up
 *      front, because the caller wants the format immediately.
 *
 *  Arguments:
 *      LPCSTR [in]: file name, or NULL when a handle is supplied.
 *      HANDLE [in]: open file handle, or INVALID_HANDLE_VALUE.
 *      BOOL [in]: asynchronous mode.  Unused: this decoder reads on demand.
 *      DWORD [in]: lookahead buffer size.
 *      DWORD [in]: maximum packets.  Unused.
 *      DWORD [in]: yield rate.  Unused: decoding is synchronous.
 *      LPWAVEFORMATEX [out]: receives the decoded PCM format.  May be NULL.
 *      XWmaFileMediaObject ** [out]: receives the object.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "WmaCreateDecoderEx"

HRESULT
WmaCreateDecoderEx
(
    LPCSTR                  pszFileName,
    HANDLE                  hFile,
    BOOL                    fAsyncMode,
    DWORD                   dwLookaheadBufferSize,
    DWORD                   dwMaxPackets,
    DWORD                   dwYieldRate,
    LPWAVEFORMATEX          pwfxDecoded,
    XWmaFileMediaObject **  ppMediaObject
)
{
    CWmaMediaObject *       pMediaObject;
    HRESULT                 hr;

    DPF_ENTER();

    UNREFERENCED_PARAMETER(fAsyncMode);
    UNREFERENCED_PARAMETER(dwMaxPackets);
    UNREFERENCED_PARAMETER(dwYieldRate);

#ifdef VALIDATE_PARAMETERS

    if(!ppMediaObject)
    {
        DPF_ERROR("No media object pointer supplied");
    }

#endif // VALIDATE_PARAMETERS

    hr = HRFROMP(pMediaObject = NEW(CWmaMediaObject));

    if(SUCCEEDED(hr))
    {
        hr = pMediaObject->InitializeFile(pszFileName, hFile, 0, dwLookaheadBufferSize);
    }

    if(SUCCEEDED(hr) && pwfxDecoded)
    {
        hr = pMediaObject->GetDecodedFormat(pwfxDecoded);
    }

    if(SUCCEEDED(hr))
    {
        *ppMediaObject = ADDREF(pMediaObject);
    }

    RELEASE(pMediaObject);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  WmaCreateDecoder
 *
 *  Description:
 *      As WmaCreateDecoderEx, handing back the base file-XMO interface.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "WmaCreateDecoder"

HRESULT
WmaCreateDecoder
(
    LPCSTR                  pszFileName,
    HANDLE                  hFile,
    BOOL                    fAsyncMode,
    DWORD                   dwLookaheadBufferSize,
    DWORD                   dwMaxPackets,
    DWORD                   dwYieldRate,
    LPWAVEFORMATEX          pwfxDecoded,
    XFileMediaObject **     ppMediaObject
)
{
    XWmaFileMediaObject *   pWmaObject = NULL;
    HRESULT                 hr;

    DPF_ENTER();

    hr = WmaCreateDecoderEx(pszFileName, hFile, fAsyncMode, dwLookaheadBufferSize,
                            dwMaxPackets, dwYieldRate, pwfxDecoded, &pWmaObject);

    if(SUCCEEDED(hr))
    {
        *ppMediaObject = pWmaObject;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  WmaCreateInMemoryDecoderEx
 *
 *  Description:
 *      Creates a WMA decoder over a title-supplied in-memory data source.
 *      The title already holds the file, so the header is parsed here rather
 *      than deferred.
 *
 *  Arguments:
 *      LPFNWMAXMODATACALLBACK [in]: data callback.
 *      LPVOID [in]: callback context.
 *      DWORD [in]: yield rate.  Unused: decoding is synchronous.
 *      LPWAVEFORMATEX [out]: receives the decoded PCM format.  May be NULL.
 *      XWmaFileMediaObject ** [out]: receives the object.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "WmaCreateInMemoryDecoderEx"

HRESULT
WmaCreateInMemoryDecoderEx
(
    LPFNWMAXMODATACALLBACK  pfnCallback,
    LPVOID                  pvContext,
    DWORD                   dwYieldRate,
    LPWAVEFORMATEX          pwfxDecoded,
    XWmaFileMediaObject **  ppMediaObject
)
{
    CWmaMediaObject *       pMediaObject;
    HRESULT                 hr;

    DPF_ENTER();

    UNREFERENCED_PARAMETER(dwYieldRate);

#ifdef VALIDATE_PARAMETERS

    if(!ppMediaObject)
    {
        DPF_ERROR("No media object pointer supplied");
    }

#endif // VALIDATE_PARAMETERS

    hr = HRFROMP(pMediaObject = NEW(CWmaMediaObject));

    if(SUCCEEDED(hr))
    {
        hr = pMediaObject->InitializeCallback(pfnCallback, pvContext);
    }

    if(SUCCEEDED(hr) && pwfxDecoded)
    {
        hr = pMediaObject->GetDecodedFormat(pwfxDecoded);
    }

    if(SUCCEEDED(hr))
    {
        *ppMediaObject = ADDREF(pMediaObject);
    }

    RELEASE(pMediaObject);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/****************************************************************************
 *
 *  WmaCreateInMemoryDecoder
 *
 *  Description:
 *      As WmaCreateInMemoryDecoderEx, handing back the base XMO interface.
 *
 ****************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "WmaCreateInMemoryDecoder"

HRESULT
WmaCreateInMemoryDecoder
(
    LPFNWMAXMODATACALLBACK  pfnCallback,
    LPVOID                  pvContext,
    DWORD                   dwYieldRate,
    LPWAVEFORMATEX          pwfxDecoded,
    LPXMEDIAOBJECT *        ppMediaObject
)
{
    XWmaFileMediaObject *   pWmaObject = NULL;
    HRESULT                 hr;

    DPF_ENTER();

    hr = WmaCreateInMemoryDecoderEx(pfnCallback, pvContext, dwYieldRate, pwfxDecoded, &pWmaObject);

    if(SUCCEEDED(hr))
    {
        *ppMediaObject = pWmaObject;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}
