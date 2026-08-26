#include "bridge_k32.h"
/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
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
