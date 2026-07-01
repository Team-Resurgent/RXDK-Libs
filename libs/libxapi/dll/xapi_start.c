/*
 * xapi_start.c - the generic XAPI title startup (RXDK's equivalent of the XDK
 * mainCRTStartup). A title that uses the Xbox API is linked with this object and
 * entered at XapiTitleStartup (-e XapiTitleStartup); the author writes only
 * main() and the XAPI + CRT + TLS bring-up runs before it, invisibly.
 *
 * IMPORTANT: this is TITLE-side glue and must be compiled with the *title*
 * recipe (the same flags a sample's main.c uses) -- NOT as an internal libxapi
 * source. libxapi's force-included profile.h defines RXDK_LIBXAPI_BUILD, which
 * flips ntdef.h/win32_bridge.h to their internal type/calling-convention view;
 * compiling the startup that way makes it call XapiInitProcess/CreateThread/etc
 * through a mismatched ABI and corrupts the stack. So it lives here for locality
 * but is built via each title's extra_srcs, exactly as the old per-sample
 * xapi_boot.c was.
 *
 * (Generalized from samples/xapi-smoke/src/xapi_boot.c.)
 */
#include <xapi.h>
#include <xboxkrnl/api/ps.h>

extern const IMAGE_TLS_DIRECTORY _tls_used;

extern ULONG XapiTlsSize;
extern ULONG _tls_index;

/* libc / per-title runtime init hook; crt0 _start calls this. */
void xbox_runtime_init(void);

/* XAPI + MSVC-style CRT init entry points (resolved from libxapi at link). */
VOID __stdcall XapiApplyKernelPatches(VOID);
VOID XapiInitProcess(VOID);
void RxdkInitKernelImportPtrs(void);
void _rtinit(void);
void _cinit(void);
void __cdecl xdk_xbox_crt_early_init(void);
void __cdecl xdk_xbox_crt_startup(void);

/* Provided by the title. Varargs so main(void) or main(int,char**) both work. */
int __cdecl main(int argc, ...);

/*
 * Standard Xbox device object strings. libxapi defines these as constants
 * (xapiinit.c), but XapiInitProcess reads them before the CRT has run, so the
 * startup re-initializes their ANSI_STRING buffers first -- same as the XDK's
 * boot did, and what the old xapi_boot.c relied on. Local struct view so we
 * don't need the internal COBJECT_STRING definition.
 */
typedef struct _RXDK_COBJECT_STRING {
    USHORT Length;
    USHORT MaximumLength;
    const char *Buffer;
} RXDK_COBJECT_STRING;

extern RXDK_COBJECT_STRING DDrive;
extern RXDK_COBJECT_STRING CdDevice;
extern RXDK_COBJECT_STRING MainVol;

static void XapiInitObjectStrings(void)
{
    static const char k_ddrive[] = "\\??\\D:";
    static const char k_cddevice[] = "\\Device\\CdRom0";
    static const char k_mainvol[] = "\\Device\\Harddisk0\\partition1\\";

    RtlInitAnsiString((PANSI_STRING)&DDrive, k_ddrive);
    RtlInitAnsiString((PANSI_STRING)&CdDevice, k_cddevice);
    RtlInitAnsiString((PANSI_STRING)&MainVol, k_mainvol);
}

/*
 * The real title thread. The kernel runs the XBE entry on its Phase-1 init
 * thread (tiny fixed stack); XAPI CreateThread spawns the title on a proper
 * TLS-bearing thread, exactly as the XDK's mainCRTStartup does.
 */
static DWORD __stdcall XapiTitleMain(LPVOID unused)
{
    (void)unused;

    DbgPrint("RXDK.start: kernel patches\n");
    XapiApplyKernelPatches();
    DbgPrint("RXDK.start: object strings\n");
    XapiInitObjectStrings();
    DbgPrint("RXDK.start: import ptrs\n");
    RxdkInitKernelImportPtrs();
    DbgPrint("RXDK.start: init process\n");
    XapiInitProcess();

    DbgPrint("RXDK.start: crt init\n");
    _rtinit();
    xdk_xbox_crt_early_init();
    _cinit();
    xdk_xbox_crt_startup();

    DbgPrint("RXDK.start: main\n");
    main(0, NULL, NULL);
    for (;;) {
    }
    return 0;
}

/* Compute the XAPI TLS image size / slot index from the PE TLS directory. */
static void XapiSetupTlsIndex(void)
{
    XapiTlsSize = (ULONG)((_tls_used.EndAddressOfRawData - _tls_used.StartAddressOfRawData) +
        _tls_used.SizeOfZeroFill);
    if (XapiTlsSize < RXDK_TLS_IMAGE_SIZE) {
        XapiTlsSize = RXDK_TLS_IMAGE_SIZE;
    }
    XapiTlsSize = (XapiTlsSize + 15u) & ~15u;
    XapiTlsSize += 4u;
    _tls_index = (ULONG)((int)XapiTlsSize / -4);
    *(PULONG)(_tls_used.AddressOfIndex) = _tls_index;
}

void XapiTitleStartup(void)
{
    HANDLE hThread;

    xbox_runtime_init();
    XapiSetupTlsIndex();

    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)XapiTitleMain, NULL, 0, NULL);
    if (hThread == NULL) {
        DbgPrint("RXDK.start: title thread create failed\n");
        for (;;) {
        }
    }
    CloseHandle(hThread);
}
