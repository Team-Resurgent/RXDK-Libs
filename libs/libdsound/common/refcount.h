/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Basic reference-counting base class used by the DirectSound objects.
 */

#ifndef __REFCOUNT_H__
#define __REFCOUNT_H__

#ifdef __cplusplus

namespace DirectSound
{
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

    //
    // These were plain ++/-- , which is only safe if every AddRef/Release runs
    // under the global DirectSound lock. That does not hold here: the interrupt
    // and DPC paths run at raised IRQL, where DirectSoundEnterCriticalSection
    // deliberately takes no lock at all (it returns FALSE above PASSIVE_LEVEL).
    // A lost decrement leaks the object; a doubled one frees it while still
    // referenced, which reads back as a garbage 'this'. Make the counter atomic.
    //

    __inline DWORD CRefCount::AddRef(void)
    {
        ASSERT(m_dwRefCount < ~0UL);
        return (DWORD)InterlockedIncrement((LONG *)&m_dwRefCount);
    }

    __inline DWORD CRefCount::Release(void)
    {
        LONG lRefCount;

        ASSERT(m_dwRefCount);

        lRefCount = InterlockedDecrement((LONG *)&m_dwRefCount);

        if(lRefCount <= 0)
        {
            delete this;
            return 0;
        }

        return (DWORD)lRefCount;
    }

    template <class type> type *__AddRef(type *p)
    {
        if(p)
        {
            p->AddRef();
        }

        return p;
    }
}

#define ADDREF(p) \
    DirectSound::__AddRef(p)

#define RELEASE(p) \
    { \
        if(p) \
        { \
            (p)->Release(); \
            (p) = NULL; \
        } \
    }

#endif // __cplusplus

#endif // __REFCOUNT_H__
