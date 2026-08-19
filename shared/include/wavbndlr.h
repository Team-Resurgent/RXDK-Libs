/***************************************************************************
 *
 *  Copyright (C) 11/2/2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       wavbndlr.h
 *  Content:    Wave Bundler definitions.
 *
 ****************************************************************************/

#ifndef __WAVBNDLR_H__
#define __WAVBNDLR_H__

//
// The on-disk wave bank format lives in xactwb.h, which is the header the XDK ships and
// therefore the one a title validates a bank against. This file adds only what the runtime
// needs on top of it: the expanded format union, the parsed section pointers, and the reader.
//
// It used to carry its own copy of the format at version 2 (the January-2002 trunk layout:
// one flat header, no segment table, a 1-bit format tag, and no per-entry flags). That copy
// was kept on the reasoning that the bank was a private contract between xactbld and this
// engine. It is not - the WaveBank and WaveBankStream samples open a .xwb and read the
// header themselves - so the format follows xactwb.h and the duplicate is gone.
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
