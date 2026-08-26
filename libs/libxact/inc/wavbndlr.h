/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Wave bank reader for the XACT runtime. Builds on the on-disk format in
 * xactwb.h to declare the expanded wave-format union, the parsed section
 * pointers, and the CWaveBankReader that opens a .xwb and exposes its sections.
 */

#ifndef __WAVBNDLR_H__
#define __WAVBNDLR_H__

//
// The on-disk wave bank format lives in xactwb.h, which is the header the XDK ships and
// therefore the one a title validates a bank against. This file adds only what the runtime
// needs on top of it: the expanded format union, the parsed section pointers, and the reader.
//
// The WaveBank and WaveBankStream samples open a .xwb and read the header themselves, so the
// format is taken directly from xactwb.h rather than duplicated here.
//

#include <xactwb.h>

// 
// Wave bank expanded wave format
//

typedef union _WAVEBANKUNIWAVEFORMAT
{
    WAVEFORMATEX        WaveFormatEx;
    XBOXADPCMWAVEFORMAT AdpcmWaveFormat;
} WAVEBANKUNIWAVEFORMAT, *LPWAVEBANKUNIWAVEFORMAT;

typedef const WAVEBANKUNIWAVEFORMAT *LPCWAVEBANKUNIWAVEFORMAT;

//
// Wave bank section data
//

typedef struct _WAVEBANKSECTIONDATA
{
    LPWAVEBANKHEADER    pHeader;            // File header
    LPWAVEBANKDATA      pBankData;          // Bank data segment (entry count, name, alignment)
    LPWAVEBANKENTRY     paMetaData;         // Array of entry meta-data
    LPVOID              pvData;             // Wave data base address
    DWORD               dwDataSize;         // Wave data size, in bytes
} WAVEBANKSECTIONDATA, *LPWAVEBANKSECTIONDATA;

typedef const WAVEBANKSECTIONDATA *LPCWAVEBANKSECTIONDATA;

//
// Helper functions
//

EXTERN_C BOOL WaveBankExpandFormat(LPCWAVEBANKMINIWAVEFORMAT pwfxCompressed, LPWAVEBANKUNIWAVEFORMAT pwfxExpanded);
EXTERN_C BOOL WaveBankCompressFormat(LPCWAVEBANKUNIWAVEFORMAT pwfxExpanded, LPWAVEBANKMINIWAVEFORMAT pwfxCompressed);

#ifdef __cplusplus

//
// Wave bank reader object
//

class CWaveBankReader
{
private:
    LPVOID                  m_pvBaseAddress;    // Bank base address
    DWORD                   m_dwBankSize;       // Bank size, in bytes

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
