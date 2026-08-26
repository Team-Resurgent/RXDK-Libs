/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Turns on 4-byte structure packing (disabling automatic field alignment)
 * using the pack pragma push option, so that poppack.h can later restore the
 * previous packing reliably. poppack.h is the complement to this header.
 */

#if ! (defined(lint) || defined(RC_INVOKED))
#if ( _MSC_VER >= 800 ) || defined(_PUSHPOP_SUPPORTED)
#pragma warning(disable:4103)
#if !(defined( MIDL_PASS )) || defined( __midl )
#pragma pack(push,4)
#else
#pragma pack(4)
#endif
#else
#pragma pack(4)
#endif
#endif // ! (defined(lint) || defined(RC_INVOKED))
