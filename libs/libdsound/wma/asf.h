// RXDK 5849 uplift: minimal ASF (Advanced Systems Format) reader -- enough to pull the audio
// stream's WAVEFORMATEX, the file/content metadata, and the compressed WMA packets out of a .wma
// file for the XMO decoders. Video, multiple audio streams, DRM and the index objects are all out
// of scope: a .wma is one audio stream, and that is all XWmaDecoderCreateMediaObject promises.
//
// The reader is pull-based and does no I/O of its own -- the caller supplies bytes. That keeps the
// XMO free to read the file however it likes (synchronously, or from a title-supplied callback as
// WmaCreateInMemoryDecoder does) without the demuxer knowing the difference.
//
// Pure C, built in the minimal (picolibc) WMA slice.

#ifndef RXDK_DSOUND_ASF_H
#define RXDK_DSOUND_ASF_H

#ifdef __cplusplus
extern "C" {
#endif

#define ASF_MAX_STRING_CHARS 128

typedef struct AsfFileInfo
{
    // From the audio stream's WAVEFORMATEX (type-specific data of its Stream Properties object).
    unsigned short  formatTag;
    unsigned short  channels;
    unsigned int    sampleRate;
    unsigned int    avgBytesPerSec;
    unsigned short  blockAlign;
    unsigned short  bitsPerSample;
    unsigned short  extradataSize;
    unsigned char   extradata[64];      // WMA codec setup; real streams use 4-10 bytes

    // From the File Properties object.
    unsigned int    durationMs;         // play duration minus preroll
    unsigned int    preroll;            // ms of leading pad the encoder added
    unsigned int    packetSize;         // ASF data-packet size (min == max for WMA)
    unsigned int    dataPacketCount;

    // From the Content Description object (empty strings when absent).
    unsigned short  title[ASF_MAX_STRING_CHARS];
    unsigned short  author[ASF_MAX_STRING_CHARS];
    unsigned short  copyright[ASF_MAX_STRING_CHARS];
    unsigned short  description[ASF_MAX_STRING_CHARS];
    unsigned short  rating[ASF_MAX_STRING_CHARS];

    // Byte offset (from the start of the file) of the first data packet.
    unsigned int    dataOffset;
} AsfFileInfo;

// Parse the ASF header. pHeader/cbHeader must span at least the whole Header Object; pass what
// AsfPeekHeaderSize reports. Returns 0 on success.
int AsfParseHeader(const unsigned char *pHeader, unsigned int cbHeader, AsfFileInfo *pInfo);

// Read the total size of the Header Object from its first 30 bytes, so a caller knows how much to
// buffer before calling AsfParseHeader. Returns 0 and sets *pcbHeader on success.
int AsfPeekHeaderSize(const unsigned char *pPrefix, unsigned int cbPrefix, unsigned int *pcbHeader);

// Pull the compressed audio payloads out of one ASF data packet. pPacket/cbPacket is exactly one
// packetSize-sized packet. Payload bytes are appended to pOut (capacity cbOut); *pcbOut receives
// the total written. Returns 0 on success -- including when the packet holds no audio payload,
// which is normal for padding packets.
int AsfParsePacket(const AsfFileInfo *pInfo,
                   const unsigned char *pPacket, unsigned int cbPacket,
                   unsigned char *pOut, unsigned int cbOut, unsigned int *pcbOut);

#ifdef __cplusplus
}
#endif

#endif
