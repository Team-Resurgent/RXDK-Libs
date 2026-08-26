/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
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

