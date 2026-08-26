/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Restores structure packing to the state saved by the matching pshpack header,
 * re-enabling the compiler's automatic field alignment (via a pack-pop pragma).
 * Each inclusion of this file must be paired one-to-one with, and preceded by,
 * an inclusion of one of the pshpack headers.
 */

#if ! (defined(lint) || defined(RC_INVOKED))
#if ( _MSC_VER >= 800 ) || defined(_PUSHPOP_SUPPORTED)
#pragma warning(disable:4103)
#if !(defined( MIDL_PASS )) || defined( __midl )
#pragma pack(pop)
#else
#pragma pack()
#endif
#else
#pragma pack()
#endif
#endif // ! (defined(lint) || defined(RC_INVOKED))
