#ifndef _XLAUNCH_H_
#define _XLAUNCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif  // ! PAGE_SIZE

#define MAX_LAUNCH_PATH       (260 + 100)
#define TITLE_PATH_DELIMITER  OTEXT(';')

//
// This is also defined in xbox.h
//

#define MAX_LAUNCH_DATA_SIZE 3072

#define LDT_LAUNCH_DASHBOARD 1
#define LDT_NONE             0xFFFFFFFF

#define LDF_HAS_BEEN_READ    0x00000001

#ifndef XBOXKRNL_TYPES_KERNEL_H

typedef struct _LAUNCH_DATA_HEADER
{
    ULONG dwLaunchDataType;
    ULONG dwTitleId;
    CHAR  szLaunchPath[520];
    ULONG dwFlags;
} LAUNCH_DATA_HEADER, *PLAUNCH_DATA_HEADER;

typedef struct _LAUNCH_DATA_PAGE
{
    LAUNCH_DATA_HEADER Header;
    UCHAR Pad[PAGE_SIZE - MAX_LAUNCH_DATA_SIZE - sizeof(LAUNCH_DATA_HEADER)];
    UCHAR LaunchData[MAX_LAUNCH_DATA_SIZE];
} LAUNCH_DATA_PAGE, *PLAUNCH_DATA_PAGE;

#if !defined(_NTSYSTEM_) && !defined(RXDK_LIBXAPI_BUILD)
extern PLAUNCH_DATA_PAGE *LaunchDataPage;
#else
extern PLAUNCH_DATA_PAGE LaunchDataPage;
#endif

#endif /* !XBOXKRNL_TYPES_KERNEL_H */

#ifdef __cplusplus
}
#endif

#endif // ! _XLAUNCH_H_
