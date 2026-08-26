/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * C entry point over the ported ffmpeg WMA decoder, callable from the C++
 * engine.
 */

#ifndef RXDK_XACT_WMABRIDGE_H
#define RXDK_XACT_WMABRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Decode a whole WMA (v1/v2) wave-bank entry to interleaved signed-16-bit PCM.
//   formatTag: 0x0160 = WMAv1, 0x0161 = WMAv2 (anything else treated as WMAv2)
//   extradata/extradataSize: the WAVEFORMATEX cbSize bytes (WMA codec setup)
//   wmaData/wmaDataSize: the raw WMA packet stream (a whole number of blockAlign packets)
//   *ppPcm: receives a malloc'd PCM buffer (free with XactWmaFree); *pcbPcm: its byte size
// Returns 0 on success, non-zero on failure.
int XactWmaDecode(unsigned short formatTag,
                  int channels, int sampleRate, int bitRate, int blockAlign,
                  const unsigned char *extradata, int extradataSize,
                  const unsigned char *wmaData, int wmaDataSize,
                  short **ppPcm, int *pcbPcm);

void XactWmaFree(short *pPcm);

// If pvData is a private "RXWM" WMA wave-bank container (xactbld emits this when a bank has WMA
// entries), decode all entries to PCM and rebuild a standard .xwb (WBND) in memory: *ppOut receives
// a malloc'd buffer (free with XactWmaFreeBank), *pcbOut its size, and returns 1. If pvData is a
// plain buffer, returns 0 and the caller uses it unchanged.
int XactMaybeTranscodeWmaBank(const unsigned char *pvData, unsigned int dwSize, void **ppOut, unsigned int *pcbOut);
void XactWmaFreeBank(void *pBank);

#ifdef __cplusplus
}
#endif

#endif
