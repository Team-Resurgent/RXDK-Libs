/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * scmemory.c -- scan-converter memory-management module.
 *
 * Sub-allocates the client work buffer into the horizontal and vertical
 * scan-array pools used during scan conversion. The two-pool split also lets
 * a client disable dropout control by giving the vertical pool zero size.
 */

/*********************************************************************/

/*      Imports                                                      */

/*********************************************************************/

#define FSCFG_INTERNAL

#include    "fscdefs.h"             /* shared data types */
#include    "scglobal.h"            /* structures & constants */
#include    "scmemory.h"            /* for own function prototypes */


/**********************************************************************

    Workspace memory is divided into two pools, identified here as
    HMem (horizontal memory) and VMem (vertical memory).  HMem is
    always used, and contains horizontal scan array lists.  VMem is 
    used only when dropout control is enabled, and contains the 
    vertical scan array lists and the contour elements used to compute 
    subpixel intersections for smart dropout control.  This division 
    into two pools was done, in part, for backward compatiblity with 
    the Apple rasterizer.  It allows a client to force dropout control
    off by setting the allocated size of VMem to zero.

**********************************************************************/
    
/*********************************************************************/

/*      Export Functions      */

/*********************************************************************/

FS_PUBLIC void fsc_SetupMem( 
        PSTATE                           /* pointer to state variables */
        char* pchHBuffer,                /* pointer to horiz workspace */
        int32 lHMemSize,                 /* size of horiz workspace */
        char* pchVBuffer,                /* pointer to vert workspace */
        int32 lVMemSize )                /* size of vert workspace */
{
    STATE.pchHNextAvailable = pchHBuffer;
    STATE.pchHWorkSpaceEnd = pchHBuffer + lHMemSize;
    
    STATE.pchVNextAvailable = pchVBuffer;
    STATE.pchVWorkSpaceEnd = pchVBuffer + lVMemSize;
}


/********************************************************************/

FS_PUBLIC void *fsc_AllocHMem( 
        PSTATE                         /* pointer to state variables  */
        int32 lSize )                  /* requested size in bytes */
{
    void *pvTemp;
    
/* printf("H: %ui   ", lSize); */

    pvTemp = (void*)STATE.pchHNextAvailable;
    ALIGN(voidPtr, lSize); 
    STATE.pchHNextAvailable += lSize;
    
    Assert(STATE.pchHNextAvailable <= STATE.pchHWorkSpaceEnd);
    return pvTemp;
}

/********************************************************************/

FS_PUBLIC void *fsc_AllocVMem( 
        PSTATE                         /* pointer to state variables */
        int32 lSize )                  /* requested size in bytes */
{
    void *pvTemp;

/* printf("V: %ui   ", lSize); */
    
    pvTemp = (void*)STATE.pchVNextAvailable;
    ALIGN(voidPtr, lSize); 
    STATE.pchVNextAvailable += lSize;
    
    Assert(STATE.pchVNextAvailable <= STATE.pchVWorkSpaceEnd);
    return pvTemp;
}

/********************************************************************/
