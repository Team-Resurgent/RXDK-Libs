/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * scline.h -- line module exports.
 *
 * Declares the fsc_ line setup and scan entry points.
 */

#include "fscdefs.h"                /* for type definitions */


/*********************************************************************/

/*              Export Functions                                     */

/*********************************************************************/

FS_PUBLIC void fsc_SetupLine ( PSTATE0 );

FS_PUBLIC int32 fsc_CalcLine( 
		PSTATE              /* pointer to state variables */
		F26Dot6,            /* point 1  x coordinate */
		F26Dot6,            /* point 1  y coordinate */
		F26Dot6,            /* point 2  y coordinate */
		F26Dot6,            /* point 2  y coordinate */
		uint16              /* scan control type */
);

/*********************************************************************/

