/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Miscellaneous NT-style driver helper functions and objects shared by the
 * DirectSound components.
 */

#ifndef __DRVHLP_H__
#define __DRVHLP_H__

#if defined(_XBOX) && defined(__cplusplus)

// 
// Raised IRQL object
//

namespace DirectSound
{
    class CIrql
    {
    private:
        KIRQL                   m_irql;
        BOOL                    m_fRaised;

    public:
        CIrql(void);

    public:
        void Raise(void);
        void Lower(void);
    };

    __inline CIrql::CIrql(void)
    {
        m_fRaised = FALSE;
    }
    
    __inline void CIrql::Raise(void)
    {
        if(m_fRaised = (KeGetCurrentIrql() < DISPATCH_LEVEL))
        {
            m_irql = KfRaiseIrql(DISPATCH_LEVEL);
        }
    }

    __inline void CIrql::Lower(void)
    {
        if(m_fRaised)
        {
            KfLowerIrql(m_irql);
            m_fRaised = FALSE;
        }
    }
}

//
// Automatic (function-scope) raised IRQL
//

namespace DirectSound
{
    class CAutoIrql
        : public CIrql
    {
    public:
        CAutoIrql(void);
        ~CAutoIrql(void);
    };

    __inline CAutoIrql::CAutoIrql(void)
    {
        Raise();
    }

    __inline CAutoIrql::~CAutoIrql(void)
    {
        Lower();
    }
}

#define AutoIrql() \
    DirectSound::CAutoIrql __AutoIrql

//
// Floating point state
//

namespace DirectSound
{
    class CFpState
    {
    private:
        static DWORD            m_dwRefCount;
        static KFLOATING_SAVE   m_fps;

    public:
        void Save(void);
        void Restore(void);
    };

    __inline void CFpState::Save(void)
    {
        if(KeIsExecutingDpc())
        {
            if(!m_dwRefCount++)
            {
                KeSaveFloatingPointState(&m_fps);
            }
        }
    }

    __inline void CFpState::Restore(void)
    {
        if(KeIsExecutingDpc())
        {
            if(!--m_dwRefCount)
            {
                KeRestoreFloatingPointState(&m_fps);
            }
        }
    }
}

//
// Automatic (function-scope) floating-point state
//

namespace DirectSound
{
    class CAutoFpState
        : private CFpState
    {
    public:
        CAutoFpState(void);
        ~CAutoFpState(void);
    };

    __inline CAutoFpState::CAutoFpState(void)
    {
        Save();
    }

    __inline CAutoFpState::~CAutoFpState(void)
    {
        Restore();
    }
}

#define AutoFpState() \
    DirectSound::CAutoFpState __AutoFpState

#endif // defined(_XBOX) && defined(__cplusplus)

#ifdef __cplusplus

//
// Interlocked and/or operations
//

// RXDK: originally MSVC __asm { or/and word/dword ptr [ecx], dx/edx } -- which
// hardcodes the __fastcall arg registers (ecx=dst, edx=src). That is correct
// only when the helper is a real CALL; when clang INLINES it (at any -O) nothing
// guarantees dst/src are in ecx/edx at the asm site, so it reads whatever garbage
// is in those registers and corrupts memory (bug hunted to here: every -Os
// libdsound crash was a bad `this`/member pointer surfacing at one of these).
//
// Replacing the asm was right, but "the asm was never atomic (no LOCK), so plain
// C is equivalent" was not: `or [mem], reg` is a SINGLE instruction, and an
// interrupt cannot be taken inside one. On this uniprocessor it was therefore
// atomic against the ISR without needing a LOCK prefix. Plain `*dst |= src`
// compiles to load/or/store, and the APU interrupt can land between those three
// and have its update dropped.
//
// These helpers maintain m_dwStatus, which the voice ISR and thread context both
// write (the ISR clears VOICEOFF to say a voice really stopped). A lost bit there
// strands the voice as still-active and wedges stream teardown. Use a real atomic
// read-modify-write so the guarantee is explicit rather than incidental.
static void __fastcall and(volatile unsigned short *dst, unsigned short src)
{
    __sync_fetch_and_and(dst, src);
}

static void __fastcall or(volatile unsigned short *dst, unsigned short src)
{
    __sync_fetch_and_or(dst, src);
}

static void __fastcall and(volatile unsigned long *dst, unsigned long src)
{
    __sync_fetch_and_and(dst, src);
}

static void __fastcall or(volatile unsigned long *dst, unsigned long src)
{
    __sync_fetch_and_or(dst, src);
}

#endif // __cplusplus

#endif // __DRVHLP_H__
