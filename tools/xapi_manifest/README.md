# xAPI build manifest

Module boundaries mirror [RXDK-LibsOld `gen-xapi.ps1`](../../scripts/gen-xapi.ps1).

Regenerate compile source lists:

```powershell
python tools/generate_xapi_manifest.py
```

Requires `vendor/xbox_private/` (run `scripts/sync-xapi-vendor.ps1`).

Output: [`build/xapi_sources.zig`](../build/xapi_sources.zig) — consumed by [`build/xapi.zig`](../build/xapi.zig).

## Internal slices (merged into `libxapi.lib`)

| Slice | NT `sources` file |
|-------|-------------------|
| k32 | `vendor/xbox_private/private/ntos/xapi/k32/lib/sources` |
| dll | `vendor/xbox_private/private/ntos/xapi/dll/sources` |
| rtl | `vendor/xbox_private/private/ntos/rtl/xapi/sources` |
| uuid | `vendor/xbox_private/private/genx/types/uuid/sources` |
| ohcd | `vendor/xbox_private/private/ntos/dd/usb/ohcd/sources` |
| usbd | `vendor/xbox_private/private/ntos/dd/usb/usbd/sources` |
| usbhub | `vendor/xbox_private/private/ntos/dd/usb/usbhub/sources` |
| mu | `vendor/xbox_private/private/ntos/dd/usb/mu/sources` |
| xid | `vendor/xbox_private/private/ntos/dd/usb/xid/sources` |

Gap fixes from RXDK-LibsOld `xapi_gap_src` are **not** a separate module — edit the owning slice sources under `vendor/xbox_private/`.
