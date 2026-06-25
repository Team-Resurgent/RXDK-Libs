#ifndef __XBOXKRNL_WINNT_XBE_H__
#define __XBOXKRNL_WINNT_XBE_H__

#include <xboxkrnl/winnt/pe.h>

typedef struct _XBE_CERTIFICATE_HEADER {
    DWORD SizeOfHeader;
    DWORD TimeDateStamp;
    DWORD TitleID;
    WCHAR TitleName[40];
    DWORD AlternateTitleIDs[16];
    DWORD AllowedMedia;
    DWORD GameRegion;
    DWORD GameRatings;
    DWORD DiscNumber;
    DWORD Version;
    BYTE LANKey[16];
    BYTE SignatureKey[16];
    BYTE AlternateSignatureKeys[16 * 16];
} XBE_CERTIFICATE_HEADER, *PXBE_CERTIFICATE_HEADER;

typedef struct _XBE_LIBRARY_HEADER {
    CHAR LibraryName[8];
    WORD MajorVersion;
    WORD MinorVersion;
    WORD BuildVersion;
    WORD LibraryFlags;
} XBE_LIBRARY_HEADER, *PXBE_LIBRARY_HEADER;

typedef struct _XBE_FILE_HEADER {
    DWORD Magic;
    UCHAR Signature[256];
    DWORD ImageBase;
    DWORD SizeOfHeaders;
    DWORD SizeOfImage;
    DWORD SizeOfImageHeader;
    DWORD TimeDateStamp;
    PXBE_CERTIFICATE_HEADER CertificateHeader;
    DWORD NumberOfSections;
    PXBE_SECTION_HEADER PointerToSectionTable;
    DWORD InitFlags;
    DWORD AddressOfEntryPoint;
    PIMAGE_TLS_DIRECTORY_32 PointerToTlsDirectory;
    DWORD SizeOfStack;
    DWORD SizeOfHeapReserve;
    DWORD SizeOfHeapCommit;
    DWORD PeImageBase;
    DWORD PeSizeOfImage;
    DWORD PeImageCheckSum;
    DWORD PeTimeDateStamp;
    DWORD PeDebugPath;
    DWORD PeDebugFilename;
    DWORD PeDebugFilenameUnicode;
    DWORD PointerToKernelThunkTable;
    DWORD PointerToDebugImportTable;
    DWORD NumberOfLibraries;
    PXBE_LIBRARY_HEADER PointerToLibraries;
    PXBE_LIBRARY_HEADER PointerToKernelLibrary;
    PXBE_LIBRARY_HEADER PointerToXapiLibrary;
    DWORD PointerToLogoBitmap;
    DWORD SizeOfLogoBitmap;
} XBE_FILE_HEADER, *PXBE_FILE_HEADER;

#define XBE_DEFAULT_BASE   (0x10000)
#define CURRENT_XBE_HEADER ((PXBE_FILE_HEADER)XBE_DEFAULT_BASE)

#endif
