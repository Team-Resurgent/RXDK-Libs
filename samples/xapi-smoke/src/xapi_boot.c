#include <xapi.h>
#include <xboxkrnl/api/ps.h>

#include "common.h"

extern const IMAGE_TLS_DIRECTORY _tls_used;
extern char _tls_start;
extern char _tls_end;

#ifndef XBEIMAGE_STANDARD_BASE_ADDRESS
#define XBEIMAGE_STANDARD_BASE_ADDRESS 0x00010000u
#endif

typedef struct _XBEIMAGE_HEADER {
    ULONG Signature;
    UCHAR EncryptedDigest[256]; // XBE digital signature is 256 bytes (2048-bit RSA).
                                // Using 260 shifts every field +4: SizeOfStackCommit
                                // then reads dwPeHeapReserve (1MB) instead of the real
                                // 64KB stack, and PsCreateSystemThreadEx gets a 1MB
                                // kernel stack -> a non-zero TLS carve overflows the
                                // kernel-stack pool and the app thread never runs.
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

    DbgPrint("xapi-smoke: TRACE app routine entered\n");
    XapiApplyKernelPatches();
    DbgPrint("xapi-smoke: TRACE after XapiApplyKernelPatches\n");
    xapi_smoke_fixup_object_strings();
    DbgPrint("xapi-smoke: TRACE after fixup_object_strings\n");
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

VOID __stdcall XapiThreadStartup(
    PKSTART_ROUTINE StartRoutine,
    PVOID StartContext);

void xapi_smoke_boot_entry(void)
{
    HANDLE hThread;
    NTSTATUS status;

    xbox_runtime_init();
    DbgPrint("xapi-smoke: boot entry\n");

    setup_xapi_tls_index();

    DbgPrint("xapi-smoke: TRACE creating app thread via CreateThread TlsSize=%lu\n",
        (unsigned long)XapiTlsSize);
    (void)status;
    // Use the XAPI CreateThread (exactly what the XDK's mainCRTStartup does) rather
    // than a raw PsCreateSystemThreadEx -- CreateThread sets the thread up the way
    // the kernel expects for a TLS-bearing title thread.
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)xapi_smoke_main_startup, NULL, 0, NULL);

    if (hThread == NULL) {
        DbgPrint("xapi-smoke: CreateThread failed\n");
        for (;;) {
        }
    }

    DbgPrint("xapi-smoke: app thread started\n");
    CloseHandle(hThread);
}
