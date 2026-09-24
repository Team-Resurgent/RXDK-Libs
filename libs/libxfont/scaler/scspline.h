/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * scspline.h -- spline module exports.
 *
 * Declares the fsc_ spline setup and scan entry points and the spline
 * length limit.
 */

#include "fscdefs.h"                /* for type definitions */


/*********************************************************************/

/*              Export Functions                                     */

/*********************************************************************/

FS_PUBLIC void fsc_SetupSpline ( PSTATE0 );

FS_PUBLIC int32 fsc_CalcSpline( 
		PSTATE          /* pointer to state varables */
		F26Dot6,        /* start point x coordinate */
		F26Dot6,        /* start point y coordinate */
		F26Dot6,        /* control point x coordinate */
		F26Dot6,        /* control point y coordinate */
		F26Dot6,        /* ending x coordinate */
		F26Dot6,        /* ending y coordinate */
		uint16          /* scan control type */
);

/********************************************************************/

/*              Export Definitions                                  */

/********************************************************************/

#define MAXSPLINELENGTH     3200        /* calculation overflow limit */

/********************************************************************/
