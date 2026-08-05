# Recovering source art out of a packed `.xpr`

A `.xpr` is bundler *output*, but it is also the only surviving copy of some
sample art. Where a source image is missing and the `.xpr` built from it is
checked in, the pixels are still there and can be lifted back out.

`extract_cubemap_face.py` does that for a cubemap face. It was written for one
case: **Water's `SkyYP.bmp`** — the +Y (up) face of its skybox — which is absent
from all three sample sources (our import, the 5849 XDK's own `Samples\Xbox`,
and the leak's `private/atg/samples`) while the other five faces survive.

    python extract_cubemap_face.py <folder containing sky.xpr>

## The part worth copying

The script **refuses to write anything until it has reproduced the faces that
still exist**. It extracts all five surviving faces, compares them byte-for-byte
against the checked-in `.bmp`, and only then writes the missing one. An
extractor that gets the swizzle or the face order wrong would produce a
plausible-looking image, so being able to check it against known-good data first
is what makes the result trustworthy rather than merely likely.

For `SkyYP.bmp` the result was then confirmed the other way round too: rebuilding
`sky.xpr` from the six `.bmp` with the RXDK bundler reproduces the original
**byte for byte, 0 of 6,293,504 bytes different**. That simultaneously proves the
recovered face is exactly what went in, and independently validates the managed
bundler's cubemap path against retail output.

## Layout notes (confirmed by arithmetic, not assumed)

- `sky.rdf` declares Cubemap, 512x512, `D3DFMT_A8R8G8B8`, no `Levels` line, so
  one level. Six faces at 512*512*4 = 6,291,456 bytes; the file is 6,293,504;
  the difference is a 2048-byte header.
- Face order is the order `Cubemap.SaveToBundle` writes them: XP, XN, YP, YN,
  ZP, ZN.
- `A8R8G8B8` is a swizzled format, so each face is Morton-ordered and has to be
  un-swizzled with the `GetMasks2` lanes for a square surface.

Generalising past cubemaps (plain 2D textures, volumes, differing formats or mip
counts) means reading the resource header rather than relying on the arithmetic
above — worth doing if another asset turns out to be recoverable this way.
