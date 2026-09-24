/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * scbitmap.h -- bitmap module exports.
 *
 * Declares the fsc_ bitmap primitives (mask init, bit access, clear and
 * gray-row calculation) used by the scan converter.
 */

#include "fscdefs.h"                /* for type definitions */


/*********************************************************************/

/*      Export Functions                                             */

/*********************************************************************/


FS_PUBLIC void fsc_InitializeBitMasks (
		void
);

FS_PUBLIC int32 fsc_ClearBitMap ( 
		uint32,             /* longs per bmp */
		uint32*             /* bitmap ptr caste long */
);

FS_PUBLIC int32 fsc_BLTHoriz ( 
		int32,              /* x start */
		int32,              /* x stop */
		uint32*             /* bit map row pointer */
);

FS_PUBLIC int32 fsc_BLTCopy ( 
		uint32*,            /* source row pointer */
		uint32*,            /* destination row pointer */
		int32               /* long word counter */
);

FS_PUBLIC uint32 fsc_GetBit ( 
		int32,              /* x coordinate */
		uint32*             /* bit map row pointer */
);

FS_PUBLIC int32 fsc_SetBit ( 
		int32,              /* x coordinate */
		uint32*             /* bit map row pointer */
);

FS_PUBLIC int32 fsc_CalcGrayRow(
		GrayScaleParam*     /* pointer to param block */
);


/*********************************************************************/
