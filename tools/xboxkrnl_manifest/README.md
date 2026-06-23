# xboxkrnl manifest

Shape templates for `tools/generate_xboxkrnl_headers.py`. **Edit layout and indentation here** — the generator strips comment banners, overlays leak values, and writes `include/xboxkrnl/`.

| Manifest | Shipped as |
|----------|------------|
| `xboxdef.h` | `include/xboxkrnl/xboxdef.h` |
| `ntstatus.h` | `include/xboxkrnl/ntstatus.h` |
| `xboxkrnl.h` | `types/{common,file,kernel,io,misc}.h` + `api/*.h` + umbrella |
| `winnt_pe.h` | `include/xboxkrnl/winnt/pe.h` |
| `winnt_xbe.h` | `include/xboxkrnl/winnt/xbe.h` |

Umbrella headers: `types.h`, `winnt.h`, `xboxkrnl.h`.

Do not `#include` from here in application code.

## Formatting

- 4-space indent inside structs, unions, and enums.
- One blank line between unrelated typedef/struct blocks.
- Keep `#define` groups contiguous.
- Type sections in `xboxkrnl.h` use `// RXDK_TYPES: name` markers (do not remove).
