#include "common.h"

static void init_object_string(OBJECT_STRING* os, char* buf)
{
    const USHORT len = (USHORT)lstrlenA(buf);
    os->Length = len;
    os->MaximumLength = (USHORT)(len + 1);
    os->Buffer = buf;
}

static int append_cstr(char* buf, int n, int cap, const char* s)
{
    if (!s) {
        return n;
    }
    for (; *s != '\0' && n + 1 < cap; ++s) {
        buf[n++] = *s;
    }
    return n;
}

void xapi_smoke_trace_line(const char* msg)
{
    char buf[128];
    int n = append_cstr(buf, 0, (int)sizeof(buf), "xapi-smoke: ");
    n = append_cstr(buf, n, (int)sizeof(buf), msg);
    if (n + 1 < (int)sizeof(buf)) {
        buf[n++] = '\n';
        buf[n] = '\0';
        OutputDebugStringA(buf);
    }
}

void xapi_smoke_trace_line2(const char* a, const char* b)
{
    char buf[128];
    int n = append_cstr(buf, 0, (int)sizeof(buf), "xapi-smoke: ");
    n = append_cstr(buf, n, (int)sizeof(buf), a);
    n = append_cstr(buf, n, (int)sizeof(buf), b);
    if (n + 1 < (int)sizeof(buf)) {
        buf[n++] = '\n';
        buf[n] = '\0';
        OutputDebugStringA(buf);
    }
}

void xapi_smoke_trace_fail(const char* name, unsigned code)
{
    char buf[128];
    int n = append_cstr(buf, 0, (int)sizeof(buf), "xapi-smoke: FAILED ");
    n = append_cstr(buf, n, (int)sizeof(buf), name);
    n = append_cstr(buf, n, (int)sizeof(buf), " code n=");
    if (n + 4 < (int)sizeof(buf)) {
        if (code >= 100u) {
            buf[n++] = (char)('0' + (code / 100u) % 10u);
        }
        if (code >= 10u) {
            buf[n++] = (char)('0' + (code / 10u) % 10u);
        }
        buf[n++] = (char)('0' + code % 10u);
        buf[n++] = '\n';
        buf[n] = '\0';
        OutputDebugStringA(buf);
    }
}

void xapi_smoke_trace_count(const char* label, unsigned value)
{
    char buf[96];
    int n = append_cstr(buf, 0, (int)sizeof(buf), "xapi-smoke: ");
    n = append_cstr(buf, n, (int)sizeof(buf), label);
    if (n + 12 < (int)sizeof(buf)) {
        buf[n++] = ' ';
        buf[n++] = 'n';
        buf[n++] = '=';
        if (value >= 100u) {
            buf[n++] = (char)('0' + (value / 100u) % 10u);
        }
        if (value >= 10u) {
            buf[n++] = (char)('0' + (value / 10u) % 10u);
        }
        buf[n++] = (char)('0' + value % 10u);
        buf[n++] = '\n';
        buf[n] = '\0';
        OutputDebugStringA(buf);
    }
}

BOOL xapi_smoke_root_is_directory(const char* root)
{
    const DWORD att = GetFileAttributesA(root);
    return att != INVALID_FILE_ATTRIBUTES && (att & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

BOOL xapi_smoke_try_mount_c_on_hdd(void)
{
    static char dosName[] = "\\??\\C:";
    static char deviceName[] = "\\Device\\Harddisk0\\Partition2";
    OBJECT_STRING dosLink;
    OBJECT_STRING deviceLink;

    init_object_string(&dosLink, dosName);
    init_object_string(&deviceLink, deviceName);
    const NTSTATUS status = IoCreateSymbolicLink(&dosLink, &deviceLink);
    return NT_SUCCESS(status) || status == STATUS_OBJECT_NAME_COLLISION;
}

BOOL xapi_smoke_build_path(char* out, int out_cap, const char* root, const char* leaf)
{
    int n = 0;
    if (!out || out_cap <= 0 || !root || !leaf) {
        return FALSE;
    }
    for (; root[n] != '\0' && n + 1 < out_cap; ++n) {
        out[n] = root[n];
    }
    for (int i = 0; leaf[i] != '\0' && n + 1 < out_cap; ++i, ++n) {
        out[n] = leaf[i];
    }
    out[n] = '\0';
    return n > 0;
}

BOOL xapi_smoke_can_create_scratch_file(const char* path)
{
    DeleteFileA(path);

    HANDLE file = CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    CloseHandle(file);
    DeleteFileA(path);
    return TRUE;
}

BOOL xapi_smoke_pick_writable_path(char* path, int path_cap, const char* leaf)
{
    static const char* const kRoots[] = {
        XAPI_SMOKE_VOLUME_ROOT,
        "T:\\",
        "U:\\",
        "C:\\",
    };

    xapi_smoke_try_mount_c_on_hdd();

    for (unsigned i = 0; i < sizeof(kRoots) / sizeof(kRoots[0]); ++i) {
        const char* root = kRoots[i];
        if (!xapi_smoke_root_is_directory(root)) {
            continue;
        }
        if (!xapi_smoke_build_path(path, path_cap, root, leaf)) {
            continue;
        }
        if (xapi_smoke_can_create_scratch_file(path)) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL xapi_smoke_try_mount_mu(char drive_out[4])
{
    if (!drive_out) {
        return FALSE;
    }
    drive_out[0] = '\0';

    if (XMountMU(XDEVICE_PORT0, XDEVICE_NO_SLOT, drive_out) != ERROR_SUCCESS) {
        return FALSE;
    }
    if (drive_out[0] == '\0' || drive_out[1] != ':') {
        (void)XUnmountMU(XDEVICE_PORT0, XDEVICE_NO_SLOT);
        drive_out[0] = '\0';
        return FALSE;
    }
    drive_out[2] = '\\';
    drive_out[3] = '\0';
    return TRUE;
}

BOOL xapi_smoke_pick_save_root(char root_out[8])
{
    static const char kUData[] = "U:\\";
    char muDrive[4];

    if (!root_out) {
        return FALSE;
    }
    root_out[0] = '\0';

    if (xapi_smoke_root_is_directory(kUData)) {
        lstrcpynA(root_out, kUData, 8);
        return TRUE;
    }

    if (xapi_smoke_try_mount_mu(muDrive)) {
        lstrcpynA(root_out, muDrive, 8);
        return TRUE;
    }

    return FALSE;
}

BOOL xapi_smoke_save_root_uses_mu(const char* root)
{
    if (!root || root[0] == '\0' || root[1] != ':') {
        return FALSE;
    }
    return root[0] != 'U';
}

void xapi_smoke_unmount_mu_port0(void)
{
    (void)XUnmountMU(XDEVICE_PORT0, XDEVICE_NO_SLOT);
}

static void xapi_smoke_format_hex8(char out[9], unsigned long value)
{
    static const char kHex[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; --i) {
        out[i] = kHex[value & 0xFu];
        value >>= 4;
    }
    out[8] = '\0';
}

BOOL xapi_smoke_pick_content_root(char root_out[8])
{
    static const char kTRoot[] = "T:\\";

    if (!root_out) {
        return FALSE;
    }
    root_out[0] = '\0';

    if (xapi_smoke_root_is_directory(kTRoot)) {
        lstrcpynA(root_out, kTRoot, 8);
        return TRUE;
    }

    return FALSE;
}

BOOL xapi_smoke_build_content_package_path(
    char* out,
    int out_cap,
    const char* t_root,
    unsigned long offering_id,
    unsigned long content_flags)
{
    char offering_hex[9];
    char flags_hex[9];
    char leaf[20];

    if (!out || out_cap <= 0 || !t_root) {
        return FALSE;
    }

    xapi_smoke_format_hex8(offering_hex, offering_id);
    xapi_smoke_format_hex8(flags_hex, content_flags);

    {
        int n = 0;
        const char* parts[] = { offering_hex, ".", flags_hex, "\\" };
        for (unsigned p = 0; p < sizeof(parts) / sizeof(parts[0]); ++p) {
            for (const char* s = parts[p]; *s != '\0' && n + 1 < (int)sizeof(leaf); ++s) {
                leaf[n++] = *s;
            }
        }
        leaf[n] = '\0';
    }

    if (!xapi_smoke_build_path(out, out_cap, t_root, "$C\\")) {
        return FALSE;
    }

    {
        int n = (int)lstrlenA(out);
        for (const char* s = leaf; *s != '\0' && n + 1 < out_cap; ++s) {
            out[n++] = *s;
        }
        out[n] = '\0';
        return n > 0;
    }
}
