// RXDK 5849 uplift: streaming (packet-at-a-time) front end over the ported ffmpeg WMA v1/v2
// decoder. wmabridge.h's XactWmaDecode decodes a whole buffer in one call, which suits XACT's
// decode-on-load wave banks but not the XMO decoders (XWmaDecoderCreateMediaObject and friends),
// which must hand back a bounded amount of PCM per Process() call while the title keeps rendering.
//
// Pure C, built in the minimal (picolibc) WMA slice alongside the decoder itself.

#ifndef RXDK_DSOUND_WMASTREAM_H
#define RXDK_DSOUND_WMASTREAM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WmaStreamDecoder WmaStreamDecoder;

// Open a decoder over the WAVEFORMATEX-shaped parameters of a WMA stream.
//   formatTag: 0x0160 = WMAv1, anything else treated as WMAv2 (0x0161)
//   extradata/extradataSize: the WAVEFORMATEX cbSize bytes (WMA codec setup)
// Returns 0 on success, non-zero on failure. blockAlign is the compressed packet size; every
// buffer handed to WmaStreamDecode must be exactly that long.
int WmaStreamOpen(unsigned short formatTag,
                  int channels, int sampleRate, int bitRate, int blockAlign,
                  const unsigned char *extradata, int extradataSize,
                  WmaStreamDecoder **ppDecoder);

void WmaStreamClose(WmaStreamDecoder *pDecoder);

// Upper bound, in bytes, on the PCM one WmaStreamDecode call can emit. Callers size their output
// buffer with this; the XMO reports it as XMEDIAINFO::dwOutputSize.
int WmaStreamMaxOutputBytes(const WmaStreamDecoder *pDecoder);

// Decode exactly one compressed packet to interleaved signed-16-bit PCM.
//   packet/packetSize: one blockAlign-sized compressed packet
//   pcmOut/pcmOutBytes: destination, at least WmaStreamMaxOutputBytes() long
//   *pcbPcm receives the bytes written (may be 0 -- the first packet of a
//   bit-reservoir stream produces no output)
// Returns 0 on success, non-zero on failure.
int WmaStreamDecode(WmaStreamDecoder *pDecoder,
                    const unsigned char *packet, int packetSize,
                    short *pcmOut, int pcmOutBytes, int *pcbPcm);

// Drop decoder history. Required after a seek: WMA's bit reservoir carries state across packets,
// so decoding a packet from elsewhere in the stream without this produces garbage.
void WmaStreamReset(WmaStreamDecoder *pDecoder);

#ifdef __cplusplus
}
#endif

#endif
