/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * scendpt.h -- endpoint module exports.
 *
 * Declares the fsc_ endpoint setup and measurement entry points.
 */

#include "fscdefs.h"                /* for type definitions */


/*********************************************************************/

/*              Export Functions                                     */

/*********************************************************************/

FS_PUBLIC void fsc_SetupEndPt ( PSTATE0 );

FS_PUBLIC void fsc_BeginContourEndpoint( 
		PSTATE              /* pointer to state variables */
		F26Dot6,            /* starting point x coordinate */
		F26Dot6             /* starting point y coordinate */
);

FS_PUBLIC int32 fsc_CheckEndPoint( 
		PSTATE              /* pointer to state variables */
		F26Dot6,            /* x coordinate */
		F26Dot6,            /* y coordinate */
		uint16              /* scan control type */
);

FS_PUBLIC int32 fsc_EndContourEndpoint( 
		PSTATE              /* pointer to state variables */
		uint16              /* scan control type */
);

/*********************************************************************/
