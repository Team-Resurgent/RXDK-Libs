/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * scendpt.c -- scan-converter endpoint module.
 *
 * Emits and checks contour endpoints, feeding scan intersections for
 * dropout-controlled filling.
 */

/*********************************************************************/

/*      Imports                                                      */

/*********************************************************************/

#define FSCFG_INTERNAL

#include    "fscdefs.h"             /* shared data types */
#include    "fserror.h"             /* error codes */

#include    "scglobal.h"            /* structures & constants */
#include    "scanlist.h"            /* saves scan line intersections */
#include    "scendpt.h"             /* for own function prototypes */

/*********************************************************************/

/*      Local Prototypes                                             */

/*********************************************************************/

FS_PRIVATE void CheckHorizTopology( PSTATE F26Dot6, F26Dot6, uint16 );
FS_PRIVATE void CheckVertTopology( PSTATE F26Dot6, F26Dot6, uint16 );

FS_PRIVATE void AddHorizOn( PSTATE uint16 );
FS_PRIVATE void AddHorizOff( PSTATE uint16 );
FS_PRIVATE void AddVertOn( PSTATE uint16 );
FS_PRIVATE void AddVertOff( PSTATE uint16 );

FS_PRIVATE F26Dot6 CalcHorizEpSubpix( int32, F26Dot6*, F26Dot6* );
FS_PRIVATE F26Dot6 CalcVertEpSubpix( int32, F26Dot6*, F26Dot6* );


/*********************************************************************/

/*      Export Functions                                             */

/*********************************************************************/

/*  pass callback routine pointers to scanlist for smart dropout control */

FS_PUBLIC void fsc_SetupEndPt (PSTATE0) 
{
	fsc_SetupCallBacks(ASTATE SC_ENDPTCODE, CalcHorizEpSubpix, CalcVertEpSubpix);
}

/*********************************************************************/

FS_PUBLIC void fsc_BeginContourEndpoint( 
		PSTATE               /* pointer to state variables */
		F26Dot6 fxX,         /* starting point x coordinate */
		F26Dot6 fxY )        /* starting point y coordinate */
{
	STATE.fxX1 = fxX;                   /* last = contour start point */
	STATE.fxY1 = fxY;
	STATE.fxX0 = HUGEFIX;               /* contour begin alert */
}


/*********************************************************************/

FS_PUBLIC int32 fsc_CheckEndPoint( 
		PSTATE               /* pointer to state variables */
		F26Dot6 fxX2,        /* x coordinate */
		F26Dot6 fxY2,        /* y coordinate */
		uint16 usScanKind )  /* dropout control type */
{
	if (ONSCANLINE(STATE.fxY1))             /* if y1 is on scan line */
	{
		if ((STATE.fxX1 == fxX2) && (STATE.fxY1 == fxY2)) /* catch dup'd points */
		{
			return NO_ERR;                  /*   and just ignore them   */
		}
				
		if (STATE.fxX0 == HUGEFIX)          /* if contour begin */
		{
			STATE.fxX2Save = fxX2;          /*   keep for contour end   */
			STATE.fxY2Save = fxY2;          
		}
		else                                /* if mid contour */
		{
			CheckHorizTopology(ASTATE fxX2, fxY2, usScanKind);
		}               
	}
	
	if (!(usScanKind & SK_NODROPOUT))       /* if dropout control on */
	{
		if (ONSCANLINE(STATE.fxX1))         /* if x1 is on scan line */
		{
			if ((STATE.fxX1 == fxX2) && (STATE.fxY1 == fxY2)) /* catch dup'd points */
			{
				return NO_ERR;              /*   and just ignore them   */
			}
				
			if (STATE.fxX0 == HUGEFIX)      /* if contour begin */
			{
				STATE.fxX2Save = fxX2;      /*   keep for contour end   */
				STATE.fxY2Save = fxY2;
			}
			else                            /* if mid contour */
			{
				CheckVertTopology(ASTATE fxX2, fxY2, usScanKind);
			}               
		}
	}

	STATE.fxX0 = STATE.fxX1;                /* old = last */
	STATE.fxY0 = STATE.fxY1;
	STATE.fxX1 = fxX2;                      /* last = current */
	STATE.fxY1 = fxY2;
	
	return NO_ERR;
}


/*********************************************************************/

FS_PUBLIC int32 fsc_EndContourEndpoint( 
		PSTATE                          /* pointer to state variables */
		uint16 usScanKind )             /* dropout control type */
{
	if (ONSCANLINE(STATE.fxY1))             /* if y1 is on scan line */
	{
		CheckHorizTopology(ASTATE STATE.fxX2Save, STATE.fxY2Save, usScanKind);
	}
	
	if (!(usScanKind & SK_NODROPOUT))       /* if dropout control on */
	{
		if (ONSCANLINE(STATE.fxX1))         /* if x1 is on scan line */
		{
			CheckVertTopology(ASTATE STATE.fxX2Save, STATE.fxY2Save, usScanKind);
		}
	}
	return NO_ERR;
}

/*********************************************************************/

/*      Private Functions      */

/*********************************************************************/

/*      Implement the endpoint-on-horiz-scanline case table    */

FS_PRIVATE void CheckHorizTopology(PSTATE F26Dot6 fxX2, F26Dot6 fxY2, uint16 usScanKind)
{

/* printf("(%li, %li)", fxX2, fxY2); */

	if (fxY2 > STATE.fxY1)
	{
		if (STATE.fxY1 > STATE.fxY0)
		{
			AddHorizOn(ASTATE usScanKind);
		}
		else if (STATE.fxY1 < STATE.fxY0)
		{
			AddHorizOn(ASTATE usScanKind);
			AddHorizOff(ASTATE usScanKind);
		}
		else                    /* (STATE.fxY1 == STATE.fxY0) */
		{
			if (STATE.fxX1 < STATE.fxX0)
			{
				AddHorizOn(ASTATE usScanKind);
			}
		}
	}
	else if (fxY2 < STATE.fxY1)
	{
		if (STATE.fxY1 > STATE.fxY0)
		{
			AddHorizOn(ASTATE usScanKind);
			AddHorizOff(ASTATE usScanKind);
		}
		else if (STATE.fxY1 < STATE.fxY0)
		{
			AddHorizOff(ASTATE usScanKind);
		}
		else                    /* (STATE.fxY1 == STATE.fxY0) */
		{
			if (STATE.fxX1 > STATE.fxX0)
			{
				AddHorizOff(ASTATE usScanKind);
			}
		}
	}
	else                        /* (fxY2 == STATE.fxY1) */
	{
		if (STATE.fxY1 > STATE.fxY0)
		{
			if (fxX2 > STATE.fxX1)
			{
				AddHorizOn(ASTATE usScanKind);
			}
		}
		else if (STATE.fxY1 < STATE.fxY0)
		{
			if (fxX2 < STATE.fxX1)
			{
				AddHorizOff(ASTATE usScanKind);
			}
		}
		else                    /* (STATE.fxY1 == STATE.fxY0) */
		{
			if ((STATE.fxX1 > STATE.fxX0) && (fxX2 < STATE.fxX1))
			{
				AddHorizOff(ASTATE usScanKind);
			}
			else if ((STATE.fxX1 < STATE.fxX0) && (fxX2 > STATE.fxX1))
			{
				AddHorizOn(ASTATE usScanKind);
			}
		}
	}
}


/*********************************************************************/

/*      Implement the endpoint-on-vert-scanline case table      */

FS_PRIVATE void CheckVertTopology(PSTATE F26Dot6 fxX2, F26Dot6 fxY2, uint16 usScanKind)
{
	if (fxX2 < STATE.fxX1)
	{
		if (STATE.fxX1 < STATE.fxX0)
		{
			AddVertOn(ASTATE usScanKind);
		}
		else if (STATE.fxX1 > STATE.fxX0)
		{
			AddVertOn(ASTATE usScanKind);
			AddVertOff(ASTATE usScanKind);
		}
		else                    /* (STATE.fxX1 == STATE.fxX0) */
		{
			if (STATE.fxY1 < STATE.fxY0)
			{
				AddVertOn(ASTATE usScanKind);
			}
		}
	}
	else if (fxX2 > STATE.fxX1)
	{
		if (STATE.fxX1 < STATE.fxX0)
		{
			AddVertOn(ASTATE usScanKind);
			AddVertOff(ASTATE usScanKind);
		}
		else if (STATE.fxX1 > STATE.fxX0)
		{
			AddVertOff(ASTATE usScanKind);
		}
		else                    /* (STATE.fxX1 == STATE.fxX0) */
		{
			if (STATE.fxY1 > STATE.fxY0)
			{
				AddVertOff(ASTATE usScanKind);
			}
		}
	}
	else                        /* (fxX2 == STATE.fxX1) */
	{
		if (STATE.fxX1 < STATE.fxX0)
		{
			if (fxY2 > STATE.fxY1)
			{
				AddVertOn(ASTATE usScanKind);
			}
		}
		else if (STATE.fxX1 > STATE.fxX0)
		{
			if (fxY2 < STATE.fxY1)
			{
				AddVertOff(ASTATE usScanKind);
			}
		}
		else                    /* (STATE.fxX1 == STATE.fxX0) */
		{
			if ((STATE.fxY1 > STATE.fxY0) && (fxY2 < STATE.fxY1))
			{
				AddVertOff(ASTATE usScanKind);
			}
			else if ((STATE.fxY1 < STATE.fxY0) && (fxY2 > STATE.fxY1))
			{
				AddVertOn(ASTATE usScanKind);
			}
		}
	}
}


/*********************************************************************/
	
FS_PRIVATE void AddHorizOn( PSTATE uint16 usScanKind )
{
	int32 lXScan, lYScan;
	void (*pfnAddHorizScan)(PSTATE int32, int32);
	void (*pfnAddVertScan)(PSTATE int32, int32);
	
	fsc_BeginElement( ASTATE usScanKind, 1, SC_ENDPTCODE,   /* quadrant and what */
					  0, NULL, NULL,                        /* number of pts */
					  &pfnAddHorizScan, &pfnAddVertScan );  /* what to call */
	
	lXScan = (int32)((STATE.fxX1 + SUBHALF - 1) >> SUBSHFT);
	lYScan = (int32)(STATE.fxY1 >> SUBSHFT);
	
	pfnAddHorizScan(ASTATE lXScan, lYScan);
}


/*********************************************************************/
	
FS_PRIVATE void AddHorizOff( PSTATE uint16 usScanKind )
{
	int32 lXScan, lYScan;
	void (*pfnAddHorizScan)(PSTATE int32, int32);
	void (*pfnAddVertScan)(PSTATE int32, int32);
	
	fsc_BeginElement( ASTATE usScanKind, 4, SC_ENDPTCODE,   /* quadrant and what */
					  0, NULL, NULL,                        /* number of pts */
					  &pfnAddHorizScan, &pfnAddVertScan );  /* what to call */

	lXScan = (int32)((STATE.fxX1 + SUBHALF) >> SUBSHFT);
	lYScan = (int32)(STATE.fxY1 >> SUBSHFT);
	
	pfnAddHorizScan(ASTATE lXScan, lYScan);
}


/*********************************************************************/
	
FS_PRIVATE void AddVertOn( PSTATE uint16 usScanKind )
{
	int32 lXScan, lYScan;
	void (*pfnAddHorizScan)(PSTATE int32, int32);
	void (*pfnAddVertScan)(PSTATE int32, int32);
	
	fsc_BeginElement( ASTATE usScanKind, 2, SC_ENDPTCODE,   /* quadrant and what */
					  0, NULL, NULL,                        /* number of pts */
					  &pfnAddHorizScan, &pfnAddVertScan );  /* what to call */

	lYScan = (int32)((STATE.fxY1 + SUBHALF - 1) >> SUBSHFT);
	lXScan = (int32)(STATE.fxX1 >> SUBSHFT);
	
	pfnAddVertScan(ASTATE lXScan, lYScan);
}


/*********************************************************************/
	
FS_PRIVATE void AddVertOff( PSTATE uint16 usScanKind )
{
	int32 lXScan, lYScan;
	void (*pfnAddHorizScan)(PSTATE int32, int32);
	void (*pfnAddVertScan)(PSTATE int32, int32);
	
	fsc_BeginElement( ASTATE usScanKind, 1, SC_ENDPTCODE,   /* quadrant and what */
					  0, NULL, NULL,                        /* number of pts */
					  &pfnAddHorizScan, &pfnAddVertScan );  /* what to call */

	lYScan = (int32)((STATE.fxY1 + SUBHALF) >> SUBSHFT);
	lXScan = (int32)(STATE.fxX1 >> SUBSHFT);
	
	pfnAddVertScan(ASTATE lXScan, lYScan);
}


/*********************************************************************/

/*      Private Callback Functions                                   */

/*********************************************************************/

FS_PRIVATE F26Dot6 CalcHorizEpSubpix(int32 lYScan, 
									 F26Dot6 *pfxX, 
									 F26Dot6 *pfxY )
{
	FS_UNUSED_PARAMETER(lYScan);
	FS_UNUSED_PARAMETER(pfxY);

/* printf("HorizEndpt(%li %li)\n", *pfxX, *pfxY); */

	return *pfxX;                           /* exact intersection */
}


/*********************************************************************/

FS_PRIVATE F26Dot6 CalcVertEpSubpix(int32 lXScan, 
									F26Dot6 *pfxX, 
									F26Dot6 *pfxY )
{
	FS_UNUSED_PARAMETER(lXScan);
	FS_UNUSED_PARAMETER(pfxX);

/* printf("VertEndpt (%li %li)\n", *pfxX, *pfxY); */

	return *pfxY;                           /* exact intersection */
}


/*********************************************************************/
