/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
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