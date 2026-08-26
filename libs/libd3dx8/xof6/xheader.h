/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * .X file header definitions: the magic, version and format constants that
 * identify a DirectX .X file and its encoding.
 */

#ifndef _XHEADER_H_
#define _XHEADER_H_

#define XFILE_MAGIC \
        ((long)'x' + ((long)'o' << 8) + ((long)'f' << 16) + ((long)' ' << 24))

#define XFILE_VERSION \
        ((long)'0' + ((long)'3' << 8) + ((long)'0' << 16) + ((long)'3' << 24))

#define XFILE_VERSION_DX3 \
        ((long)'0' + ((long)'3' << 8) + ((long)'0' << 16) + ((long)'2' << 24))

#define XFILE_VERSION_TEXT "0303"

#define XFILE_FORMAT_BINARY \
        ((long)'b' + ((long)'i' << 8) + ((long)'n' << 16) + ((long)' ' << 24))

#define XFILE_FORMAT_TEXT   \
        ((long)'t' + ((long)'x' << 8) + ((long)'t' << 16) + ((long)' ' << 24))

#define XFILE_FORMAT_TEXT_ZIP \
        ((long)'t' + ((long)'z' << 8) + ((long)'i' << 16) + ((long)'p' << 24))

#define XFILE_FORMAT_BINARY_ZIP \
        ((long)'b' + ((long)'z' << 8) + ((long)'i' << 16) + ((long)'p' << 24))

#define XFILE_FLOATSIZE_32 \
        ((long)'0' + ((long)'0' << 8) + ((long)'3' << 16) + ((long)'2' << 24))

#define XFILE_FLOATSIZE_32_TEXT "0032"

#define XFILE_FLOATSIZE_64 \
        ((long)'0' + ((long)'0' << 8) + ((long)'6' << 16) + ((long)'4' << 24))

#endif // _XHEADER_H_
