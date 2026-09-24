/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 *
 * <sys/sendfile.h> -- picolibc ships none. sendfile() copies between two
 * descriptors through a bounce buffer (the console has no zero-copy path).
 * Implemented in runtime/xbox/fileio.c.
 */
#ifndef _RXDK_SYS_SENDFILE_H_
#define _RXDK_SYS_SENDFILE_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* _RXDK_SYS_SENDFILE_H_ */
