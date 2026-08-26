/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Top-level NT API include file. Pulling in this header defines the public NT
 * types and system-call declarations that an application or subsystem written
 * to the NT API can use; it aggregates the individual nt*api.h sub-headers.
 */

#pragma once
#define NT_INCLUDED


#if defined (_MSC_VER)
#if ( _MSC_VER >= 800 )
#pragma warning(disable:4514)
#ifndef __cplusplus
#pragma warning(disable:4116)       // TYPE_ALIGNMENT generates this - move it
                                    // outside the warning push/pop scope.
#endif
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4001)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#endif
#if (_MSC_VER > 1020)
#pragma once
#endif
#endif
//
//  Common definitions
//

#define _CTYPE_DISABLE_MACROS

#include <excpt.h>
#include <stdarg.h>
#include <ntdef.h>

#include <ntstatus.h>
#include <ntkeapi.h>

#ifdef _X86_
#include "nti386.h"
#endif // i386

//
//  Each NT Component that exports system call APIs to user programs
//  should have its own include file included here.
//

#include <ntseapi.h>
#include <ntobapi.h>
#include <ntimage.h>
#include <ntldr.h>
#include <ntpsapi.h>
#include <ntxcapi.h>
#include <ntlpcapi.h>
#include <ntioapi.h>
#include <ntpoapi.h>
#include <ntexapi.h>
#include <ntmmapi.h>
#include <ntregapi.h>
#include <ntconfig.h>
#include <ntnls.h>

#if defined (_MSC_VER)
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4001)
#pragma warning(default:4201)
#pragma warning(default:4214)
#endif
#endif

