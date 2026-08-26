/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Band instrument objects for the DirectMusic band track.
 *
 * CBandInstrument holds the MIDI patch/bank, pan, volume and channel-priority
 * settings for one instrument within a band, and knows how to load its DLS
 * instrument onto a port. CDownloadedInstrument tracks an instrument that has
 * been downloaded to a port so it can be unloaded again when the band is torn
 * down.
 */

#include "pchdmband.h"

//////////////////////////////////////////////////////////////////////
// Class CDownloadedInstrument

//////////////////////////////////////////////////////////////////////
// CDownloadedInstrument::CDownloadedInstrument
#ifdef DXAPI
CDownloadedInstrument::~CDownloadedInstrument()
{
	if(m_pDLInstrument)
	{
		if (m_pPort)
        {
            if (FAILED(m_pPort->UnloadInstrument(m_pDLInstrument)))
            {
                Trace(1,"Error: UnloadInstrument failed\n");    
            }
        }
        m_pDLInstrument->Release();
	}

	if(m_pPort)
	{
		m_pPort->Release();
	}
}
#endif

//////////////////////////////////////////////////////////////////////
// Class CBandInstrument

//////////////////////////////////////////////////////////////////////
// CBandInstrument::CBandInstrument

CBandInstrument::CBandInstrument() 
{
    m_dwPatch = 0;
    m_dwAssignPatch = 0;
    m_bPan = 0;
    m_bVolume = 0;
    m_dwPChannel = 0;
    m_dwFlags = 0;
    m_nTranspose = 0;
    m_fGMOnly = false;
    m_fNotInFile = false;
    m_pIDMCollection = NULL;
    m_dwChannelPriority = 0;
    m_dwFullPatch = 0;
    m_nPitchBendRange = 0;
	ZeroMemory(m_dwNoteRanges, sizeof(m_dwNoteRanges));
}

//////////////////////////////////////////////////////////////////////
// CBandInstrument::~CBandInstrument

CBandInstrument::~CBandInstrument()
{
	if(m_pIDMCollection)
	{
		m_pIDMCollection->Release();
	}
}
#ifdef DXAPI
void CDownloadList::Clear()

{
    CDownloadedInstrument *pDownload;
    while (pDownload = RemoveHead())
    {
        delete pDownload;
    }
}

#endif