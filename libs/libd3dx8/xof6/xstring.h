/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Declares XString, a simple owning string wrapper.
 */

#ifndef _XSTRING_H_
#define _XSTRING_H_

class XString
{
    CHAR *m_szData;

public:
    XString(const CHAR *szSrc = NULL);

    ~XString();

    const XString& operator=(const CHAR *szSrc);

    operator const CHAR *() const { return m_szData; }
};

#define xstricmp _stricmp
#define xstrcmp strcmp
#define xstrlen strlen
CHAR *xstrdup(const CHAR *s);

#endif // _XSTRING_H_
