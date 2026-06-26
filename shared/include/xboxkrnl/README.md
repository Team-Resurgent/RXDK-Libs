# xboxkrnl headers

```c
#include <xboxkrnl/xboxkrnl.h>
```

Optional granular includes:

```c
#include <xboxkrnl/types/common.h>
#include <xboxkrnl/api/ke.h>
#include <xboxkrnl/winnt/pe.h>
```

Regenerate:

```powershell
python tools/generate_xboxkrnl_headers.py
```

Link with `prebuilt/xboxkrnl.lib`.

These headers are exercised by being compiled and linked into `libc`, `libxapi`,
and the `xapi-smoke` / `libc-smoke` / `libcpp-smoke` samples (all of which link
`prebuilt/xboxkrnl.lib`). The standalone full-export probe samples
(`kernel-api-link` / `-probe`) were retired in the tree cleanup.

## Layout

| Header | Role |
|--------|------|
| `xboxdef.h` | Scalars and portable base types (`DWORD`, `STRING`, `LIST_ENTRY`, `CONTEXT`, …) |
| `ntstatus.h` | `STATUS_*` codes |
| `types/common.h` | Calling conventions, macros, strings, list helpers |
| `types/file.h` | File-system structs and enums |
| `types/kernel.h` | Executive / scheduler types (`KEVENT`, `KTHREAD`, …) |
| `types/io.h` | I/O types (`IRP`, `DEVICE_OBJECT`, …) |
| `types/misc.h` | MM/crypto/HAL/IDE helpers and remaining types |
| `types.h` | Umbrella over `types/*.h` |
| `api/*.h` | Kernel import declarations by subsystem |
| `winnt/pe.h` | PE image formats |
| `winnt/xbe.h` | XBE image formats |
| `winnt.h` | Umbrella over `winnt/*.h` |
| `xboxkrnl.h` | Full kernel API umbrella |

Formatting policy: edit manifests under `tools/xboxkrnl_manifest/`; the generator strips comment banners and overlays leak values only.
