/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * xbld.c -- the XBOXKRNL library-version record, in source.
 *
 * Every Xbox title's PE must carry a .XBLD section: a table of XBE_LIBRARY_VERSION
 * records (one per XDK library it was built with) that imagebld copies into the
 * XBE header's "library versions" table, and that identifies the build to the
 * loader/kernel. This is the XBOXKRNL record, exposed as XboxKrnlBuildNumber.
 *
 * It replaces the former binary blob prebuilt/xboxkrnl_xbld.obj (the XDK's
 * private\ntos\init\console\bldnum.obj) -- byte-identical 16-byte record, now
 * built from source so the tree carries no prebuilt objects. It is packed into
 * libc.lib (every title links it); libs/libc/xbox/startup.c references
 * XboxKrnlBuildNumber, which pulls this object into the link and keeps the
 * .XBLD$V section from being GC'd.
 */

#pragma pack(push, 1)
typedef struct {
    char           szName[8];      /* library name, space/NUL padded to 8 */
    unsigned short wMajorVersion;
    unsigned short wMinorVersion;
    unsigned short wBuildVersion;
    unsigned short dwFlags;        /* QFEVersion:13 | Approved:2 | DebugBuild:1 */
} XBE_LIBRARY_VERSION;
#pragma pack(pop)

/*
 * Grouped section ".XBLD$V" so the linker sorts/merges every library's record
 * into one contiguous .XBLD (the $-suffix convention the XDK linker uses).
 * `used` keeps it even though nothing in this TU reads it.
 */
__attribute__((section(".XBLD$V"), used))
const XBE_LIBRARY_VERSION XboxKrnlBuildNumber = {
    { 'X', 'B', 'O', 'X', 'K', 'R', 'N', 'L' },
    1,       /* major */
    0,       /* minor */
    6800,    /* build (0x1a90) */
    0x4001,  /* flags */
};
