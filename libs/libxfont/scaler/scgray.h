/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * scgray.h -- grayscale parameter-block definition.
 *
 * Declares the gray-scale calculation parameters passed between the
 * scan-converter modules for grayscale rendering.
 */

#ifndef FSCGRAY_DEFINED
#define FSCGRAY_DEFINED


#include "fscdefs.h"                /* for type definitions */

/*********************************************************************/

/*      Gray scale calculation parameters                            */

/*********************************************************************/

typedef struct
{
	char* pchOver;                  /* pointer to overscaled bitmap */
	char* pchGray;                  /* pointer to gray scale bitmap */
	int16 sGrayCol;                 /* number of gray columns to calc */
	uint16 usOverScale;             /* outline magnification factor */
	uint16 usFirstShift;            /* first byte's shift */
	char* pchOverLo;                /* low limit of overscaled bitmap */
	char* pchOverHi;                /* high limit of overscaled bitmap */
	char* pchGrayLo;                /* low limit of gray scale bitmap */
	char* pchGrayHi;                /* high limit of gray scale bitmap */
}
GrayScaleParam;


/*********************************************************************/

#endif
