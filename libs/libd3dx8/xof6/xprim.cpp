/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Implements XPrimitiveList: the table mapping .X primitive type names (WORD,
 * DWORD, FLOAT and so on) to their type enum and byte size.
 */

#include "precomp.h"

static const XPrimitive aPrimitives[] = {
    "WORD",      X_Word,      sizeof(unsigned short),
    "DWORD",     X_DWord,     sizeof(unsigned long),
    "FLOAT",     X_Float,     sizeof(float),
    "DOUBLE",    X_Double,    sizeof(double),
    "CHAR",      X_Char,      sizeof(char),
    "UCHAR",     X_UChar,     sizeof(unsigned char),
    "BYTE",      X_UChar,     sizeof(unsigned char),
    "SWORD",     X_SWord,     sizeof(short),
    "SDWORD",    X_SDWord,    sizeof(long),
    "STRING",    X_Lpstr,     sizeof(char *),
    "CSTRING",   X_Cstring,   sizeof(char *),
    "UNICODE",   X_Unicode,   sizeof(short *),
    "ULONGLONG", X_ULongLong, sizeof(__int64)
};

static const int cPrimitives = sizeof(aPrimitives)/sizeof(XPrimitive);

const XPrimitive *XPrimitiveFromName(LPCSTR szName)
{
    for (DWORD i = 0; i < cPrimitives; i++) {
        if (!xstricmp(szName, aPrimitives[i].name))
            return &aPrimitives[i];
    }
    return NULL;
}
