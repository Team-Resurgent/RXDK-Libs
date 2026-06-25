#include <xapi/pe32.h>
#include <xboxkrnl/xboxkrnl.h>
#include <xboxkrnl/api/ps.h>

#include "common.h"
#include "xapi_tls_layout.h"

extern const IMAGE_TLS_DIRECTORY _tls_used;
extern char _tls_start;
extern char _tls_end;

#ifndef XBEIMAGE_STANDARD_BASE_ADDRESS
#define XBEIMAGE_STANDARD_BASE_ADDRESS 0x00010000u
#endif

typedef struct _XBEIMAGE_HEADER {
    ULONG Signature;
    UCHAR EncryptedDigest[260];
    PVOID BaseAddress;
    ULONG SizeOfHeaders;
    ULONG SizeOfImage;
    ULONG SizeOfImageHeader;
    ULONG TimeDateStamp;
    PVOID Certificate;
    ULONG NumberOfSections;
    PVOID SectionHeaders;
    ULONG InitFlags;
    PVOID AddressOfEntryPoint;
    PIMAGE_TLS_DIRECTORY TlsDirectory;
    ULONG SizeOfStackCommit;
    ULONG SizeOfHeapReserve;
    ULONG SizeOfHeapCommit;
} XBEIMAGE_HEADER, *PXBEIMAGE_HEADER;

#define XeImageHeader() ((PXBEIMAGE_HEADER)(ULONG_PTR)XBEIMAGE_STANDARD_BASE_ADDRESS)

extern ULONG XapiTlsSize;
extern ULONG _tls_index;

void xbox_runtime_init(void);

VOID __stdcall XapiApplyKernelPatches(VOID);
VOID XapiInitProcess(VOID);
void _rtinit(void);
void _cinit(void);
void __cdecl xdk_xbox_crt_early_init(void);
void __cdecl xdk_xbox_crt_startup(void);
int __cdecl main(int argc, ...);

typedef struct _RXDK_COBJECT_STRING {
    USHORT Length;
    USHORT MaximumLength;
    const char *Buffer;
} RXDK_COBJECT_STRING;

extern RXDK_COBJECT_STRING DDrive;
extern RXDK_COBJECT_STRING CdDevice;
extern RXDK_COBJECT_STRING MainVol;

static void xapi_smoke_fixup_object_strings(void)
{
    static const char k_ddrive[] = "\\??\\D:";
    static const char k_cddevice[] = "\\Device\\CdRom0";
    static const char k_mainvol[] = "\\Device\\Harddisk0\\partition1\\";

    RtlInitAnsiString((PANSI_STRING)&DDrive, k_ddrive);
    RtlInitAnsiString((PANSI_STRING)&CdDevice, k_cddevice);
    RtlInitAnsiString((PANSI_STRING)&MainVol, k_mainvol);
}

void RxdkInitKernelImportPtrs(void);

static DWORD __stdcall xapi_smoke_main_startup(LPVOID unused)
{
    (void)unused;

    XapiApplyKernelPatches();
    xapi_smoke_fixup_object_strings();
    RxdkInitKernelImportPtrs();
    DbgPrint("xapi-smoke: init process\n");
    XapiInitProcess();
    DbgPrint("xapi-smoke: crt init\n");
    _rtinit();
    xdk_xbox_crt_early_init();
    _cinit();
    xdk_xbox_crt_startup();

    DbgPrint("xapi-smoke: main\n");
    main(0, NULL, NULL);
    for (;;) {
    }
    return 0;
}

static void setup_xapi_tls_index(void)
{
    XapiTlsSize = (_tls_used.EndAddressOfRawData - _tls_used.StartAddressOfRawData) +
        _tls_used.SizeOfZeroFill;
    if (XapiTlsSize < RXDK_TLS_IMAGE_SIZE) {
        DbgPrint("xapi-smoke: TLS template %lu < %u\n",
            (unsigned long)XapiTlsSize, (unsigned)RXDK_TLS_IMAGE_SIZE);
        XapiTlsSize = RXDK_TLS_IMAGE_SIZE;
    }
    XapiTlsSize = (XapiTlsSize + 15u) & ~15u;
    XapiTlsSize += 4u;
    _tls_index = (ULONG)((int)XapiTlsSize / -4);
    *(PULONG)(_tls_used.AddressOfIndex) = _tls_index;
}

VOID XapiThreadStartup(
    PKSTART_ROUTINE StartRoutine,
    PVOID StartContext);

void xapi_smoke_boot_entry(void)
{
    HANDLE hThread;
    NTSTATUS status;

    xbox_runtime_init();
    DbgPrint("xapi-smoke: boot entry\n");

    setup_xapi_tls_index();

    status = PsCreateSystemThreadEx(
        &hThread,
        0,
        XeImageHeader()->SizeOfStackCommit,
        XapiTlsSize,
        NULL,
        (PKSTART_ROUTINE)xapi_smoke_main_startup,
        NULL,
        FALSE,
        FALSE,
        (PKSYSTEM_ROUTINE)XapiThreadStartup);

    if (!NT_SUCCESS(status)) {
        DbgPrint("xapi-smoke: PsCreateSystemThreadEx %08x\n", (unsigned)status);
        for (;;) {
        }
    }

    DbgPrint("xapi-smoke: app thread started\n");
    CloseHandle(hThread);
}
