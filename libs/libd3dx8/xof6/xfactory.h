/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declares XFactory, the IClassFactory implementation for the DirectXFile
 * object.
 */

#ifndef _XFACTORY_H_
#define _XFACTORY_H_

////////////////////////////////////////////////////////////////////////////
//
//  XFactory: IClassFactory impl class.
//
////////////////////////////////////////////////////////////////////////////

class XFactory : public IClassFactory
{
private:
    ULONG m_cRef;

public:
    XFactory() : m_cRef(1) {}

    STDMETHOD(QueryInterface) (REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    STDMETHOD(CreateInstance) (LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppv);
    STDMETHOD(LockServer) (BOOL fLock);
};

#endif // _XFACTORY_H_
