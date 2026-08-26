/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declaration of CreateSegmentFromMIDIStream, which imports a MIDI stream into
 * a DirectMusic segment.
 */

#ifndef MIDIFILE_H
#define MIDIFILE_H

HRESULT CreateSegmentFromMIDIStream(LPSTREAM pStream, 
									IDirectMusicSegment* pSegment);
#endif // #ifndef MIDIFILE_H