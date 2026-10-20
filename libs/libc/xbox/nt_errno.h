/*
 * Copyright (c) 2026 Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * NTSTATUS -> errno for the file and directory calls (fileio.c, dirio.c), so a
 * caller can tell why an open, stat, unlink, mkdir or rmdir failed: "exists"
 * (an exclusive create, mkdir), "not found", "not empty" and so on, as the
 * Windows CRT reports the same NT failures.
 */
#pragma once

#include <errno.h>

static inline int rxdk_errno_from_status(long status)
{
    switch ((unsigned long)status) {
    case 0xC000000FUL: /* STATUS_NO_SUCH_FILE */
    case 0xC0000033UL: /* STATUS_OBJECT_NAME_INVALID */
    case 0xC0000034UL: /* STATUS_OBJECT_NAME_NOT_FOUND */
    case 0xC000003AUL: /* STATUS_OBJECT_PATH_NOT_FOUND */
    case 0xC000003BUL: /* STATUS_OBJECT_PATH_SYNTAX_BAD */
        return ENOENT;
    case 0xC0000035UL: /* STATUS_OBJECT_NAME_COLLISION */
        return EEXIST;
    case 0xC0000022UL: /* STATUS_ACCESS_DENIED */
    case 0xC0000043UL: /* STATUS_SHARING_VIOLATION */
    case 0xC0000056UL: /* STATUS_DELETE_PENDING */
    case 0xC0000121UL: /* STATUS_CANNOT_DELETE */
        return EACCES;
    case 0xC0000103UL: /* STATUS_NOT_A_DIRECTORY */
        return ENOTDIR;
    case 0xC00000BAUL: /* STATUS_FILE_IS_A_DIRECTORY */
        return EISDIR;
    case 0xC0000101UL: /* STATUS_DIRECTORY_NOT_EMPTY */
        return ENOTEMPTY;
    case 0xC000007FUL: /* STATUS_DISK_FULL */
        return ENOSPC;
    case 0xC00000A2UL: /* STATUS_MEDIA_WRITE_PROTECTED */
        return EROFS;
    case 0xC000011FUL: /* STATUS_TOO_MANY_OPENED_FILES */
        return EMFILE;
    case 0xC000009AUL: /* STATUS_INSUFFICIENT_RESOURCES */
    case 0xC0000017UL: /* STATUS_NO_MEMORY */
        return ENOMEM;
    default:
        return EIO;
    }
}
