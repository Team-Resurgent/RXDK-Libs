/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declares the XPrimitive type enum and XPrimitiveList used to describe .X file
 * primitive types.
 */

#ifndef _XPRIM_H_
#define _XPRIM_H_

typedef enum _XPrimType {
    X_Word,             // 16 bits (unsigned short)
    X_DWord,            // 32 bits (unsigned long)
    X_Float,            // 32 bits (float)
    X_Double,           // 64 bits (double)
    X_Char,             // 8 bits (char)
    X_UChar,            // 8 bits (unsigned char)
    X_SWord,            // (short)
    X_SDWord,           // (long)
    X_Lpstr,            // Null terminated string
    X_Cstring,          // C string
    X_Unicode,          // Unicode string
    X_ULongLong         // 64 bit int
} XPrimType;

class XPrimitive {
public:
    LPCSTR   name;
    XPrimType   type;
    DWORD       size;
};

const XPrimitive *XPrimitiveFromName(LPCSTR szName);

#endif // _XPRIM_H_
