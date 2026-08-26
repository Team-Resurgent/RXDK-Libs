/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declaration of the IDirectMusicStyle8 interface (new for DX8).
 *
 * Extends IDirectMusicStyle with the DX8 composition entry points, including
 * ComposeMelodyFromTemplate for building a sequence segment from a style and a
 * melody template.
 */

#ifndef __DX8STYLE_H_
#define __DX8STYLE_H_

/*
@interface IDirectMusicStyle8 | 
The <i IDirectMusicStyle8> interface extends the <i IDirectMusicStyle> interface.


@base public | IUnknown

@meth HRESULT | ComposeMelodyFromTemplate | Creates a sequence segment from a 
style and a Melody template (containing a Melody Generation track, a Chord track,
and an optional Style track).  Clones the segment and adds a 
Sequence track containing melodic information.
*/

// This eventually will go in dmusici.h

/*/////////////////////////////////////////////////////////////////////*/

#endif //__DX8STYLE_H_
