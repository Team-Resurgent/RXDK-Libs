/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Precompiled header for libxapi's dll sources. Pulls in the internal XAPI
 * headers (xapip.h, xboxp.h) together with the storage/SCSI device-ioctl
 * headers and the XAPI version stamp shared across this directory.
 */

#pragma once
#define _DLLP_


#include "xapip.h"
#include <xboxp.h>
#include <stdio.h>
#include <scsi.h>
#include <ntddcdrm.h>
#include <ntddcdvd.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <dvdx2.h>

#include "xapiver.h"

