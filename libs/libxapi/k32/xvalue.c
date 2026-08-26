#include "bridge_k32.h"
/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Xbox configuration-value access backed by EEPROM non-volatile settings:
 * XSetValue / XQueryValue, plus the typed accessors for common settings -
 * language, AV pack, video standard and flags, audio flags, parental-control
 * level, game region, and the auto-logon flag.
 */

#include "basedll.h"
#include "fatx.h"
#include "av.h"
#include "xconfig.h"
#include <xboxp.h>

#ifdef XAPILIBP

extern const OBJECT_STRING XapiHardDisk;

#else  // XAPILIBP

const OBJECT_STRING XapiHardDisk = CONSTANT_OBJECT_STRING( OTEXT("\\Device\\Harddisk0\\partition0") );

#endif // XAPILIBP


#ifdef XAPILIBP

DWORD
XSetValue(
    IN ULONG ulValueIndex,
    IN ULONG ulType,
    IN PVOID pValue,
    IN ULONG cbValueLength
    )
{
    NTSTATUS status;
    status = ExSaveNonVolatileSetting(ulValueIndex, ulType, pValue, cbValueLength);
    return RtlNtStatusToDosError(status);
}

#endif // XAPILIBP

#ifndef XAPILIBP

DWORD
XQueryValue(
    IN ULONG ulValueIndex,
    OUT PULONG pulType,
    OUT PVOID pValue,
    IN ULONG cbValueLength,
    IN OUT PULONG pcbResultLength
    )
{
    NTSTATUS status;

    status = ExQueryNonVolatileSetting(
                ulValueIndex,
                pulType,
                pValue,
                cbValueLength,
                pcbResultLength);

    return RtlNtStatusToDosError(status);
}


DWORD
XGetLanguage(
    VOID
    )
{
    ULONG ulType;
    DWORD dwValue;
    return NT_SUCCESS(ExQueryNonVolatileSetting(
                         XC_LANGUAGE,
                         &ulType,
                         &dwValue,
                         sizeof(dwValue),
                         NULL)) ? dwValue : XC_LANGUAGE_UNKNOWN;
}


DWORD
XGetAVPack(
    VOID
    )
{
    return HalBootSMCVideoMode;
}


DWORD
XGetVideoStandard(
    VOID
    )
{
    ULONG ulType;
    DWORD dwValue;
    return NT_SUCCESS(ExQueryNonVolatileSetting(
                         XC_FACTORY_AV_REGION,
                         &ulType,
                         &dwValue,
                         sizeof(dwValue),
                         NULL)) ?
                          ((dwValue & AV_STANDARD_MASK) >> AV_STANDARD_BIT_SHIFT) :
                          0;
}


DWORD
XGetVideoFlags(
    VOID
    )
{
    ULONG ulType;
    DWORD dwValue = NT_SUCCESS(ExQueryNonVolatileSetting(
                               XC_VIDEO_FLAGS,
                               &ulType,
                               &dwValue,
                               sizeof(dwValue),
                               NULL)) ?
                                 ((dwValue & AV_USER_FLAGS_MASK) >> AV_USER_FLAGS_BIT_SHIFT) :
                                 0;

    switch (XGetAVPack())
    {
        case XC_AV_PACK_SCART:
        case XC_AV_PACK_VGA:
        case XC_AV_PACK_RFU:
        case XC_AV_PACK_SVIDEO:
        case XC_AV_PACK_STANDARD:

            //
            // Do not allow HDTV flags for AV packs that we know do not
            // support component out
            //

            dwValue &= ~(XC_VIDEO_FLAGS_HDTV_720p |
                         XC_VIDEO_FLAGS_HDTV_1080i |
                         XC_VIDEO_FLAGS_HDTV_480p);
            break;

        case XC_AV_PACK_HDTV:
        default:

             //
             // Allow HDTV flags for everything else
             //

             break;
    }

    return dwValue;
}


DWORD
XGetAudioFlags(
    VOID
    )
{
    ULONG ulType;
    DWORD dwValue = NT_SUCCESS(ExQueryNonVolatileSetting(
                               XC_AUDIO_FLAGS,
                               &ulType,
                               &dwValue,
                               sizeof(dwValue),
                               NULL)) ? dwValue : 0;

    switch (XGetAVPack())
    {
        case XC_AV_PACK_RFU:

            //
            // RF pack should always be mono because that is all that it can output
            //

            dwValue = XC_AUDIO_FLAGS_MONO;
            break;

        case XC_AV_PACK_STANDARD:

            //
            // Std pack has no optical output so we mask off DD & DTS bits
            //

            dwValue = XC_AUDIO_FLAGS_BASIC(dwValue);
            break;
    }

    return dwValue;
}


DWORD
XGetParentalControlSetting(
    VOID
    )
{
    ULONG ulType;
    DWORD dwValue;
    return NT_SUCCESS(ExQueryNonVolatileSetting(
                         XC_PARENTAL_CONTROL_GAMES,
                         &ulType,
                         &dwValue,
                         sizeof(dwValue),
                         NULL)) ? dwValue : 0;
}


DWORD
XGetGameRegion(
    VOID
    )
{
    ULONG ulType;
    DWORD dwValue;
    return NT_SUCCESS(ExQueryNonVolatileSetting(
                         XC_FACTORY_GAME_REGION,
                         &ulType,
                         &dwValue,
                         sizeof(dwValue),
                         NULL)) ? dwValue : 0;
}


//
// Reports whether the dashboard's auto-logon setting permits a title to sign
// the user in without prompting. Bit 0x4 of the misc-flags EEPROM setting is
// the "not allowed" bit; a failed read is treated as not allowed.
//
#define XC_MISC_FLAG_AUTO_LOGON_NOT_ALLOWED  0x0004

DWORD
__attribute__((__stdcall__))
XGetAutoLogonFlag(
    VOID
    )
{
    ULONG ulType;
    DWORD dwValue;

    if (NT_SUCCESS(ExQueryNonVolatileSetting(
                       XC_MISC_FLAGS,
                       &ulType,
                       &dwValue,
                       sizeof(dwValue),
                       NULL))
        && !(dwValue & XC_MISC_FLAG_AUTO_LOGON_NOT_ALLOWED)) {

        return XC_AUTO_LOGON_ALLOWED;
    }

    return XC_AUTO_LOGON_NOT_ALLOWED;
}

#endif // ! XAPILIBP
