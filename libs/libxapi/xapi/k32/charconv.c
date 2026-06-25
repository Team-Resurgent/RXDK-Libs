#include "bridge_k32.h"
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
