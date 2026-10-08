/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Default TZ from the console's time zone setting.
 *
 * picolibc's tzset() only reads the TZ environment variable and falls back to
 * UTC without it, so localtime() would ignore the time zone chosen in the
 * dashboard. Before main() runs, this builds the equivalent POSIX rule from the
 * EEPROM user settings -- the same fields libxapi's GetTimeZoneInformation()
 * reads -- and installs it as TZ, unless the title already set one.
 *
 * Kernel-only, like the rest of libs/libc/xbox: the settings come straight from
 * ExQueryNonVolatileSetting rather than through libxapi.
 */

/* setenv() is POSIX; picolibc hides it under a strict -std=c17. */
#define _GNU_SOURCE 1

#include <stdlib.h>
#include <string.h>
#include <sys/xbox_tz.h>

#include "xbox/kernel.h"

/* ExQueryNonVolatileSetting index that returns the whole user settings section
   (XC_MAX_OS in libxapi's xboxp.h). */
#define XC_MAX_OS                 0xff
#define XC_MISC_FLAG_DONT_USE_DST 0x0002
#define EEPROM_TOTAL_MEMORY_SIZE  256
#define XC_TZNAMELEN              4

/* EEPROM user settings section; layout from libxapi support/inc/ntos/xconfig.h. */
#pragma pack(push, 1)
typedef struct {
    UCHAR Month; /* 0 = no time zone transition */
    UCHAR Day;   /* occurrence of DayOfWeek in the month, 5 = last */
    UCHAR DayOfWeek;
    UCHAR Hour;
} xbox_timezone_date;

typedef struct {
    ULONG              Checksum;
    LONG               TimeZoneBias;
    CHAR               TimeZoneStdName[XC_TZNAMELEN];
    CHAR               TimeZoneDltName[XC_TZNAMELEN];
    ULONG              Reserved1[2];
    xbox_timezone_date TimeZoneStdDate;
    xbox_timezone_date TimeZoneDltDate;
    ULONG              Reserved2[2];
    LONG               TimeZoneStdBias;
    LONG               TimeZoneDltBias;
    ULONG              Language;
    ULONG              VideoFlags;
    ULONG              AudioFlags;
    ULONG              ParentalControlGames;
    ULONG              ParentalControlPassword;
    ULONG              ParentalControlMovies;
    ULONG              OnlineIpAddress;
    ULONG              OnlineDnsAddress;
    ULONG              OnlineDefaultGatewayAddress;
    ULONG              OnlineSubnetMask;
    ULONG              MiscFlags;
    ULONG              DvdRegion;
} xbox_user_settings;
#pragma pack(pop)

_Static_assert(sizeof(xbox_user_settings) == 96, "XBOX_USER_SETTINGS layout");

static void copy_date(struct __xbox_tz_date *dst, const xbox_timezone_date *src)
{
    dst->month = src->Month;
    dst->week = src->Day;
    dst->wday = src->DayOfWeek;
    dst->hour = src->Hour;
    dst->minute = 0;
}

void xbox_init_timezone(void)
{
    UCHAR                 buf[EEPROM_TOTAL_MEMORY_SIZE];
    ULONG                 type = 0, len = 0;
    struct __xbox_tz_info info;
    char                  rule[__XBOX_TZ_RULE_MAX];

    if (getenv("TZ"))
        return;

    if (!NT_SUCCESS(ExQueryNonVolatileSetting(XC_MAX_OS, &type, buf, sizeof(buf), &len))
        || len < sizeof(xbox_user_settings))
        return;

    const xbox_user_settings *config = (const xbox_user_settings *)buf;

    memset(&info, 0, sizeof(info));
    info.bias = config->TimeZoneBias;
    memcpy(info.std_name, config->TimeZoneStdName, XC_TZNAMELEN);
    memcpy(info.dst_name, config->TimeZoneDltName, XC_TZNAMELEN);

    /* With daylight saving turned off in the dashboard there are no transitions,
       exactly as GetTimeZoneInformation() reports it. */
    if (!(config->MiscFlags & XC_MISC_FLAG_DONT_USE_DST)) {
        info.std_bias = config->TimeZoneStdBias;
        info.dst_bias = config->TimeZoneDltBias;
        copy_date(&info.std_date, &config->TimeZoneStdDate);
        copy_date(&info.dst_date, &config->TimeZoneDltDate);
    }

    if (__xbox_tz_rule(&info, rule, sizeof(rule)) > 0)
        setenv("TZ", rule, 0);
}
