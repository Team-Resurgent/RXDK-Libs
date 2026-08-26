/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * CMIXBINS -- class wrapper around a DSMIXBINS structure, with debug-build
 * validation of mixbin assignments.
 */


#pragma once

#include <xtl.h>
#include <dsound.h>

class CMIXBINS
{
public:
    CMIXBINS(void);
    LPCDSMIXBINS GetMixBins(void);
    void         CreateFromMask(DWORD dwMask);
    void         PokeMixBinVolumesBasedOnMask(DWORD dwMixBinMask, long *alVolumes);
    BOOL         operator != (CMIXBINS &p_MixBins);
    BOOL         operator == (CMIXBINS &p_MixBins);

    #ifdef _DEBUG
    ~CMIXBINS(void);
    #endif

private:
    BOOL  m_bValid;
    DSMIXBINS m_DSMixBins;
    DSMIXBINVOLUMEPAIR m_VolumePair[DSMIXBIN_ASSIGNMENT_MAX];
};