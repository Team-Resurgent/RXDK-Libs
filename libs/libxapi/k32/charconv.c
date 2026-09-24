#include "bridge_k32.h"
/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Win32 character-case conversion helpers: CharUpper/CharLower in ANSI (A) and
 * wide (W) forms. Each handles both the string-pointer form and the packed
 * single-character form (when the high word is zero) via the Rtl case-mapping
 * primitives.
 */

#include "basedll.h"

LPSTR
__attribute__((__stdcall__))
CharUpperA(
    LPSTR psz
    )
{
    if (HIWORD(psz))
    {
        LPSTR pszCur = psz;
        while (*pszCur)
        {
            *pszCur = RtlUpperChar(*pszCur);
            pszCur++;
        }
        return psz;
    }
    else
    {
        return (LPSTR)RtlUpperChar((CHAR)psz);
    }
}

LPSTR
__attribute__((__stdcall__))
CharLowerA(
    LPSTR psz
    )
{
    if (HIWORD(psz))
    {
        LPSTR pszCur = psz;
        while (*pszCur)
        {
            *pszCur = RtlLowerChar(*pszCur);
            pszCur++;
        }
        return psz;
    }
    else
    {
        return (LPSTR)RtlLowerChar((CHAR)psz);
    }
}

LPWSTR
__attribute__((__stdcall__))
CharUpperW(
    LPWSTR psz
    )
{
    if (HIWORD(psz))
    {
        LPWSTR pszCur = psz;
        while (*pszCur)
        {
            *pszCur = RtlUpcaseUnicodeChar(*pszCur);
            pszCur++;
        }
        return psz;
    }
    else
    {
        return (LPWSTR)RtlUpcaseUnicodeChar((WCHAR)psz);
    }
}

LPWSTR
__attribute__((__stdcall__))
CharLowerW(
    LPWSTR psz
    )
{
    if (HIWORD(psz))
    {
        LPWSTR pszCur = psz;
        while (*pszCur)
        {
            *pszCur = RtlDowncaseUnicodeChar(*pszCur);
            pszCur++;
        }
        return psz;
    }
    else
    {
        return (LPWSTR)RtlDowncaseUnicodeChar((WCHAR)psz);
    }
}
