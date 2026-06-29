/*
 * RXDK minimal <tchar.h> shim (ANSI / non-UNICODE).
 *
 * Shadows zig's MinGW tchar.h (which pulls _mingw.h -> vadefs.h and fails under
 * our freestanding flags). The Xbox builds are ANSI, so the generic-text TCHAR
 * maps to char and the _t* routines map to the plain C string/printf functions.
 */
#ifndef RXDK_TCHAR_H
#define RXDK_TCHAR_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef char            TCHAR, _TCHAR;
typedef char            TBYTE;
typedef char *          PTCHAR, *PTSTR, *LPTSTR;
typedef const char *    PCTSTR, *LPCTSTR;
#endif

#ifndef __TEXT
#define __TEXT(q)   q
#endif
#ifndef _T
#define _T(x)       x
#define _TEXT(x)    x
#endif
#ifndef TEXT
#define TEXT(x)     x
#endif

/* String routines (generic-text -> ANSI). */
#define _tcslen     strlen
#define _tcscpy     strcpy
#define _tcsncpy    strncpy
#define _tcscat     strcat
#define _tcsncat    strncat
#define _tcscmp     strcmp
#define _tcsncmp    strncmp
#define _tcsicmp    strcasecmp
#define _tcsnicmp   strncasecmp
#define _tcschr     strchr
#define _tcsrchr    strrchr
#define _tcsstr     strstr
#define _tcstok     strtok
#define _tcsdup     strdup
#define _tcscpy_s(d, n, s)  strncpy((d), (s), (n))

/* Char classification / conversion. */
#define _totupper   toupper
#define _totlower   tolower
#define _istdigit   isdigit
#define _istalpha   isalpha
#define _istspace   isspace

/* Formatted I/O. */
#define _stprintf   sprintf
#define _sntprintf  snprintf
#define _vstprintf  vsprintf
#define _vsntprintf vsnprintf
#define _tprintf    printf

/* Numeric conversion. */
#define _tcstol     strtol
#define _tcstoul    strtoul
#define _tcstod     strtod
#define _ttoi       atoi
#define _ttol       atol
#define _itot(v, b, r)  itoa((v), (b), (r))

#endif /* RXDK_TCHAR_H */
