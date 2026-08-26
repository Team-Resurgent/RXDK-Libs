/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Wave Bundler runtime support. Layered on the on-disk wave-bank format in
 * <xactwb.h>, it adds what the runtime needs to consume a bank: the expanded
 * WAVEBANKUNIWAVEFORMAT (a full WAVEFORMATEX/ADPCM view of the packed mini
 * format), the parsed section pointers in WAVEBANKSECTIONDATA, format
 * expand/compress helpers, and the CWaveBankReader that maps a .xwb file.
 */

#ifndef __WAVBNDLR_H__
#define __WAVBNDLR_H__

#include <xactwb.h>

//
// Wave bank expanded wave format
//

typedef union _WAVEBANKUNIWAVEFORMAT {
    WAVEFORMATEX WaveFormatEx;
    XBOXADPCMWAVEFORMAT AdpcmWaveFormat;
} WAVEBANKUNIWAVEFORMAT, *LPWAVEBANKUNIWAVEFORMAT;

typedef const WAVEBANKUNIWAVEFORMAT* LPCWAVEBANKUNIWAVEFORMAT;

//
// Wave bank section data
//

typedef struct _WAVEBANKSECTIONDATA {
    LPWAVEBANKHEADER pHeader; // File header
    LPWAVEBANKDATA pBankData; // Bank data segment (entry count, name, alignment)
    LPWAVEBANKENTRY paMetaData; // Array of entry meta-data
    LPVOID pvData; // Wave data base address
    DWORD dwDataSize; // Wave data size, in bytes
} WAVEBANKSECTIONDATA, *LPWAVEBANKSECTIONDATA;

typedef const WAVEBANKSECTIONDATA* LPCWAVEBANKSECTIONDATA;

//
// Helper functions. Convert between the packed WAVEBANKMINIWAVEFORMAT stored in
// a bank entry and the full WAVEFORMATEX/ADPCM union DirectSound consumes.
// Return FALSE on an unsupported or malformed format.
//

EXTERN_C BOOL WaveBankExpandFormat(LPCWAVEBANKMINIWAVEFORMAT pwfxCompressed, LPWAVEBANKUNIWAVEFORMAT pwfxExpanded);
EXTERN_C BOOL WaveBankCompressFormat(LPCWAVEBANKUNIWAVEFORMAT pwfxExpanded, LPWAVEBANKMINIWAVEFORMAT pwfxCompressed);

#ifdef __cplusplus

//
// Wave bank reader object. Open() maps a .xwb file into memory; GetSectionData()
// returns pointers into that image (header, bank data, entry meta-data array,
// and wave-data base) without copying. Flush() releases the mapping.
//

class CWaveBankReader
{
  private:
    LPVOID m_pvBaseAddress; // Bank base address
    DWORD m_dwBankSize; // Bank size, in bytes

  public:
    CWaveBankReader(void);
    virtual ~CWaveBankReader(void);

  public:
    // Initialization
    HRESULT Open(LPCSTR pszBankPath);
    void Flush(void);

    // Bank data
    void GetSectionData(LPWAVEBANKSECTIONDATA pSectionData);
};

#endif // __cplusplus

#endif // __WAVBNDLR_H__
