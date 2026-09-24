/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * scmemory.h -- memory module exports.
 *
 * Declares the fsc_ workspace setup and allocation entry points for the
 * scan converter.
 */

#include "fscdefs.h"                /* for type definitions */


/*********************************************************************/

/*              Export Functions                                     */

/*********************************************************************/

FS_PUBLIC void fsc_SetupMem( 
        PSTATE              /* pointer to state variables */
        char*,              /* pointer to horiz workspace */
        int32 ,             /* size of horiz workspace */
        char*,              /* pointer to vert workspace */
        int32               /* size of vert workspace */
);

FS_PUBLIC void *fsc_AllocHMem( 
        PSTATE              /* pointer to state variables */
        int32               /* allocate from horiz memory pool */
);

FS_PUBLIC void *fsc_AllocVMem( 
        PSTATE              /* pointer to state variables */
        int32               /* allocate from vert memory pool */
);

/*********************************************************************/
