# Pixel-shader assembly for the imported XDK samples

Several XDK samples `#include` a `.inl` that assigns a `D3DPIXELSHADERDEF`
field by field, and pass it straight to `CreatePixelShader`. Those `.inl` files
are *tool output*, not hand-written art: the XDK build ran `xsasm` over the
sample's `Media\Shaders\*.psh` and checked the result in. A few samples reached
us without theirs, which is what these scripts regenerate.

## What the assembler actually emits

`xsasm`'s default output file (`.xpu`) is:

    offset 0   DWORD  0x30425350   -- the tag "PSB0"
    offset 4   ...    a verbatim D3DPIXELSHADERDEF (60 DWORDs / 240 bytes)

so an `.xpu` is always 244 bytes and needs no interpretation, only naming. That
was established empirically, not assumed -- see *Verification* below.

The shipped `.inl` files spell their values with the `PS_*` packing macros
(`PS_COMBINERCOUNT(...)`, `PS_TEXTUREMODES(...)`, ...). `psh_to_inl.py` writes
the same words as hex instead. The compiler cannot tell the two apart; the hex
form is simply what you get when you recover the words rather than the source
expression that produced them.

## Scripts

| file | what it does |
| --- | --- |
| `psh_to_inl.py <sample-dir>` | assembles every `Media\Shaders\*.psh` and writes `<base>.inl` next to the sample's sources |
| `verify_pairs.py` | proves the format against samples that shipped *both* a `.psh` and its `.inl` |
| `dump_psd.c` + `host_prelude.h` | tiny host program: `#include`s one `.inl` and prints the resulting 60 DWORDs |

`dump_psd.c` is the piece that makes verification possible at all -- it lets a
`.inl`'s macro soup be *materialised* on the host and compared as numbers.
It needs `SHADER_INL` defined to the `.inl` path, and
`-fno-sanitize=undefined` because the `PS_*` macros shift signed constants past
bit 31.

## Verification

`verify_pairs.py` covers the 11 samples that shipped both halves --
`Graphics\FocusBlur` (blur, depth, depthlookup, focus, focuslookup, Mipmap) and
`Graphics\Fur` (comp2, comp3, furfade0..2). For each it assembles the `.psh`,
compiles the *shipped* `.inl`, and diffs all 60 words.

    11/11 identical.

`psh_to_inl.py` was then round-tripped on FocusBlur: its generated `.inl` and
the shipped `.inl` compile to the same 60 words, which is what pins the field
*order* rather than just the total size.

## Cross-platform caveat

`xsasm.exe` is a Win32 PE, so these scripts only run on Windows. Regenerating a
`.inl` is a one-off -- the results are checked in, and no build step invokes the
assembler -- but authoring *new* shaders on Linux or macOS needs a managed port
of `xsasm` in RXDK-Tools. That port is tracked separately; the format notes
above are the bulk of what it needs to know for pixel shaders. Vertex shaders
(`.vsh`) are not characterised here, because every sample that uses one loads
the assembled binary at runtime instead of `#include`-ing it.
