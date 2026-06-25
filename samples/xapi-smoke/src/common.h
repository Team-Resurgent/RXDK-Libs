#pragma once

#include <xapi.h>

#define XAPI_OK   0
#define XAPI_SKIP 255

/* Retail kit / XISO boot volume (see crt-cpp17-filesystem vs deploy path). */
#define XAPI_SMOKE_VOLUME_ROOT "D:\\"
#define XAPI_SMOKE_FIND_ALL    "D:\\*"

/* One OutputDebugStringA per logical line (VS Code splits multiple ODS calls). */
void xapi_smoke_trace_line(const char* msg);
void xapi_smoke_trace_line2(const char* a, const char* b);
void xapi_smoke_trace_fail(const char* name, unsigned code);
void xapi_smoke_trace_count(const char* label, unsigned value);

BOOL xapi_smoke_root_is_directory(const char* root);
BOOL xapi_smoke_try_mount_c_on_hdd(void);
BOOL xapi_smoke_build_path(char* out, int out_cap, const char* root, const char* leaf);
BOOL xapi_smoke_can_create_scratch_file(const char* path);
BOOL xapi_smoke_pick_writable_path(char* path, int path_cap, const char* leaf);
BOOL xapi_smoke_try_mount_mu(char drive_out[4]);
BOOL xapi_smoke_pick_save_root(char root_out[8]);
BOOL xapi_smoke_save_root_uses_mu(const char* root);
void xapi_smoke_unmount_mu_port0(void);
BOOL xapi_smoke_pick_content_root(char root_out[8]);
BOOL xapi_smoke_build_content_package_path(
    char* out,
    int out_cap,
    const char* t_root,
    unsigned long offering_id,
    unsigned long content_flags);
