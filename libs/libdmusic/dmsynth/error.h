/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Private HRESULT error codes for the synthesizer's DLS/wave loader - bad
 * wave, region, articulation and collection chunks, missing loader, lock
 * failures and similar - all based off the RA_E_FIRST facility offset.
 */

#define RA_E_FIRST				(OLE_E_FIRST + 5000)

#define E_BADWAVE		        (RA_E_FIRST + 1)    // Bad wave chunk
#define E_NOTPCM		        (RA_E_FIRST + 2)    // Not PCM data in wave
#define E_NOTMONO		        (RA_E_FIRST + 3)    // Wave not MONO
#define E_BADARTICULATION       (RA_E_FIRST + 4)    // Bad articulation chunk
#define E_BADREGION		        (RA_E_FIRST + 5)    // Bad region chunk
#define E_BADWAVELINK	        (RA_E_FIRST + 6)    // Bad link from region to wave
#define E_BADINSTRUMENT	        (RA_E_FIRST + 7)    // Bad instrument chunk
#define E_NOARTICULATION        (RA_E_FIRST + 8)    // No articulation found in region
#define E_NOWAVE		        (RA_E_FIRST + 9)    // No wave found for region.
#define E_BADCOLLECTION	        (RA_E_FIRST + 10)   // Bad collection chunk.
#define E_NOLOADER   	        (RA_E_FIRST + 11)   // No IRALoader interface 
#define E_NOLOCK		        (RA_E_FIRST + 12)   // Unable to lock a region.
#define E_TOOBUSY		        (RA_E_FIRST + 13)   // RActive to busy to fully follow command.

