/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Implements XString, the simple owning string wrapper used by the parser.
 */

#include "precomp.h"

XString::XString(const CHAR *szSrc)
{
    m_szData = xstrdup(szSrc);
}

XString::~XString()
{
    if (m_szData)
        XFree((void *)m_szData);
}


const XString& XString::operator=(const CHAR *szSrc)
{
    if (m_szData)
        XFree((void *)m_szData);

    m_szData = xstrdup(szSrc);

    return *this;
}

#undef DPF_MODNAME
#define DPF_MODNAME "xstrdup"
CHAR *xstrdup(const CHAR *s)
{
    CHAR *d = NULL;

    if (s) {
        int n = (xstrlen(s) + 1) * sizeof(CHAR);

        if (SUCCEEDED(XMalloc((void **) &d, n)))
            memcpy(d, s, n);
        else
            DPF_ERR("Failed to allocate space for string");
    }

    return d;
}
