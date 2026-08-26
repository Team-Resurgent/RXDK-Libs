/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * NT-style driver helper objects and small utilities for the XACT runtime: the
 * CIrql/CAutoIrql scoped IRQL raisers, bit-counting helpers, and interlocked
 * and/or operations on volatile 16/32-bit words.
 */

#ifndef __DRVHLP_H__
#define __DRVHLP_H__

#if defined(_XBOX) && defined(__cplusplus)

// 
// Raised IRQL object
//

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

//
// Automatic (function-scope) raised IRQL
//

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

#define AutoIrql() \
    CAutoIrql __AutoIrql

#endif // defined(_XBOX) && defined(__cplusplus)

//
// Misc. helpers
//

__inline UINT CountBits(DWORD dwBits)
{
    UINT                    nCount;
    UINT                    i;

    for(i = 0, nCount = 0; i < 32; i++)
    {
        if(dwBits & (1UL << i))
        {
            nCount++;
        }
    }

    return nCount;
}

__inline UINT GetBitIndex(DWORD dwBit)
{
    UINT                    i;

    for(i = 0; i < 32; i++)
    {
        if(dwBit & (1UL << i))
        {
            break;
        }
    }

    return i;
}

#ifdef __cplusplus

//
// Interlocked and/or operations
//

static void __fastcall and(volatile unsigned short *dst, unsigned short src)
{
    __asm
    {
        and word ptr [ecx], dx
    }
}

static void __fastcall or(volatile unsigned short *dst, unsigned short src)
{
    __asm
    {
        or word ptr [ecx], dx
    }
}

static void __fastcall and(volatile unsigned long *dst, unsigned long src)
{
    __asm
    {
        and dword ptr [ecx], edx
    }                        
}

static void __fastcall or(volatile unsigned long *dst, unsigned long src)
{
    __asm
    {
        or dword ptr [ecx], edx
    }
}

#endif // __cplusplus

#endif // __DRVHLP_H__
