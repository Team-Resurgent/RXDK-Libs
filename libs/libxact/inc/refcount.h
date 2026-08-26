/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * CRefCount -- a basic reference-counting base class that deletes itself when
 * the reference count reaches zero.
 */

#ifndef __REFCOUNT_H__
#define __REFCOUNT_H__

#ifdef __cplusplus

class CRefCount
{
protected:
    DWORD                   m_dwRefCount;

public:
    CRefCount(DWORD dwInitialRefCount = 1);
    virtual ~CRefCount(void);

public:
    virtual DWORD STDMETHODCALLTYPE AddRef(void);
    virtual DWORD STDMETHODCALLTYPE Release(void);
};

__inline CRefCount::CRefCount(DWORD dwInitialRefCount)
    : m_dwRefCount(dwInitialRefCount)
{
}

__inline CRefCount::~CRefCount(void)
{
    ASSERT(!m_dwRefCount);
}

__inline DWORD CRefCount::AddRef(void)
{
    ASSERT(m_dwRefCount < ~0UL);
    return ++m_dwRefCount;
}

__inline DWORD CRefCount::Release(void)
{
    ASSERT(m_dwRefCount);

    if(m_dwRefCount > 0)
    {
        if(!--m_dwRefCount)
        {
            delete this;
            return 0;
        }
    }

    return m_dwRefCount;
}

#endif // __cplusplus

#endif // __REFCOUNT_H__
