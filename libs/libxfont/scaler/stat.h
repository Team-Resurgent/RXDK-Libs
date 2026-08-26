/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * stat.h -- profiling timing macros (stat card / IceCAP).
 *
 * Defines the STAT_ON/STAT_OFF timing macros used to instrument the
 * rasterizer. They expand to stat-card or IceCAP sampling calls when a
 * profiling build option is enabled, and to nothing otherwise.
 */

/* STAT Card Timing Switches */

#ifdef FSCFG_USESTATCARD         /* compile option for profiling */
  #define STAT_ON               gbTimer = TRUE; StartSample();
  #define STAT_OFF              StopSample(); gbTimer = FALSE;
  #define STAT_ON_CALLBACK      if (gbTimer) StartSample();
  #define STAT_OFF_CALLBACK     if (gbTimer) StopSample();
#elif FSCFG_USEICECAP          /* compile option for profiling with IceCAP */
//#include <ICAPExp.h>
  #define STAT_ON               gbTimer = TRUE; StartCAP();
  #define STAT_OFF              StopCAP(); gbTimer = FALSE;
  #define STAT_ON_CALLBACK      if (gbTimer) ResumeCAP();
  #define STAT_OFF_CALLBACK     if (gbTimer) SuspendCAP();
#else
  #define STAT_ON
  #define STAT_OFF
  #define STAT_ON_CALLBACK
  #define STAT_OFF_CALLBACK
#endif

#if 1

#define STAT_ON_NEWSFNT         STAT_ON
#define STAT_OFF_NEWSFNT        STAT_OFF
#define STAT_ON_NEWTRAN         STAT_ON
#define STAT_OFF_NEWTRAN        STAT_OFF
#define STAT_ON_NEWGLYPH        STAT_ON
#define STAT_OFF_NEWGLYPH       STAT_OFF
#define STAT_ON_GRIDFIT         STAT_ON
#define STAT_OFF_GRIDFIT        STAT_OFF
#define STAT_ON_FINDBMS         STAT_ON
#define STAT_OFF_FINDBMS        STAT_OFF
#define STAT_ON_SCAN            STAT_ON
#define STAT_OFF_SCAN           STAT_OFF
#define STAT_ON_FNTEXEC         
#define STAT_OFF_FNTEXEC        
#define STAT_ON_IUP
#define STAT_OFF_IUP
#define STAT_ON_CALCORIG
#define STAT_OFF_CALCORIG

#else

#define STAT_ON_NEWSFNT
#define STAT_OFF_NEWSFNT
#define STAT_ON_NEWTRAN
#define STAT_OFF_NEWTRAN
#define STAT_ON_NEWGLYPH
#define STAT_OFF_NEWGLYPH
#define STAT_ON_GRIDFIT         STAT_ON         
#define STAT_OFF_GRIDFIT        STAT_OFF         
#define STAT_ON_FINDBMS   
#define STAT_OFF_FINDBMS  
#define STAT_ON_SCAN            
#define STAT_OFF_SCAN           
#define STAT_ON_FNTEXEC
#define STAT_OFF_FNTEXEC
#define STAT_ON_IUP
#define STAT_OFF_IUP
#define STAT_ON_CALCORIG
#define STAT_OFF_CALCORIG

#endif

/*********************************************************************/

/*              Global timing variable                               */

/*********************************************************************/

extern boolean gbTimer;                /* set true when timer running */

/*********************************************************************/

/*              Export Functions                                     */

/*********************************************************************/


#ifdef FSCFG_USESTATCARD         /* compile option for profiling */

uint16 __cdecl  InitStat( void );

uint16 __cdecl  ConfigElapsed( void );

uint16 __cdecl  ConfigSample( void );

uint16 __cdecl  StartElapsed( void );

uint16 __cdecl  StartSample( void );

uint32 __cdecl  ReadElapsed( void );

uint32 __cdecl  ReadSample( void );

uint16 __cdecl  ReadSample_Count( void );

uint16 __cdecl  StopElapsed( void );

uint16 __cdecl  StopSample( void );

uint16 __cdecl  ResetElapsed( void );

uint16 __cdecl  ResetSample( void );

#endif  /* FSCFG_USESTATCARD */

/*********************************************************************/
