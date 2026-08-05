"""Extract a cubemap face from Water's sky.xpr back out to a 24-bit .bmp.

Why this is possible at all: SkyYP.bmp (the +Y / up face) is absent from every
sample source, but sky.xpr -- the packed resource the bundler produced from all
six faces back when they existed -- is checked in. The pixels are still there.

Layout, all confirmed by arithmetic against the file size:
  sky.rdf says Cubemap, 512x512, D3DFMT_A8R8G8B8, no Levels line -> 1 level.
  6 faces * 512*512*4 = 6,291,456; the file is 6,293,504; so 2048 bytes of
  header precede the pixel data.
  Face order is the order Cubemap.SaveToBundle writes them: XP,XN,YP,YN,ZP,ZN.

A8R8G8B8 is a swizzled Xbox format, so each face is Morton-ordered and has to be
un-swizzled. The masks come from GetMasks2 for a square 512x512 surface.

The extractor is validated against a face that DOES survive as a .bmp before the
missing one is written -- see verify().
"""
import struct, sys, os

FACE = {"XP": 0, "XN": 1, "YP": 2, "YN": 3, "ZP": 4, "ZN": 5}
SIZE = 512
BPP = 4
HDR = 2048


def masks(width, height):
    lw = width.bit_length() - 1
    lh = height.bit_length() - 1
    log = min(lw, lh)
    lower = (1 << (log << 1)) - 1
    upper = ~lower & 0xFFFFFFFF
    mu = (0x55555555 | upper) if lw > lh else (0x55555555 & lower)
    mv = (0xAAAAAAAA | upper) if lw < lh else (0xAAAAAAAA & lower)
    limit = (1 << (lw + lh)) - 1
    return mu & limit, mv & limit


def deposit(value, mask):
    out = 0
    bit = 0
    for i in range(32):
        if mask & (1 << i):
            if value & (1 << bit):
                out |= 1 << i
            bit += 1
    return out


def unswizzle_face(raw):
    """raw = one swizzled 512x512 A8R8G8B8 face -> linear BGRA rows."""
    mu, mv = masks(SIZE, SIZE)
    du = [deposit(x, mu) for x in range(SIZE)]
    dv = [deposit(y, mv) for y in range(SIZE)]
    out = bytearray(SIZE * SIZE * BPP)
    for y in range(SIZE):
        sv = dv[y]
        rowbase = y * SIZE * BPP
        for x in range(SIZE):
            src = (du[x] | sv) * BPP
            dst = rowbase + x * BPP
            out[dst:dst + BPP] = raw[src:src + BPP]
    return out


def to_bmp24(linear):
    """linear BGRA (top-down) -> 24-bit BMP bytes (bottom-up)."""
    rowbytes = SIZE * 3
    pixels = bytearray()
    for y in range(SIZE - 1, -1, -1):           # BMP rows run bottom-up
        base = y * SIZE * BPP
        row = bytearray(rowbytes)
        for x in range(SIZE):
            s = base + x * BPP
            d = x * 3
            row[d:d + 3] = linear[s:s + 3]      # B,G,R -- drop A
        pixels += row
    size = 54 + len(pixels)
    hdr = b"BM" + struct.pack("<IHHI", size, 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, SIZE, SIZE, 1, 24, 0, len(pixels), 0, 0, 0, 0)
    return hdr + dib + bytes(pixels)


def face_bytes(xpr, name):
    with open(xpr, "rb") as f:
        f.seek(HDR + FACE[name] * SIZE * SIZE * BPP)
        return f.read(SIZE * SIZE * BPP)


def verify(xpr, folder):
    """Extract every face that still has a .bmp and compare pixel-for-pixel."""
    ok = True
    for name in ("XP", "XN", "YN", "ZP", "ZN"):
        ref = os.path.join(folder, "Sky%s.bmp" % name)
        if not os.path.exists(ref):
            continue
        mine = to_bmp24(unswizzle_face(face_bytes(xpr, name)))
        theirs = open(ref, "rb").read()
        if mine[54:] == theirs[54:]:
            print("  MATCH  Sky%s.bmp" % name)
        else:
            diff = sum(1 for a, b in zip(mine[54:], theirs[54:]) if a != b)
            print("  DIFF   Sky%s.bmp  (%d of %d bytes)" % (name, diff, len(theirs) - 54))
            ok = False
    return ok


if __name__ == "__main__":
    folder = sys.argv[1]
    xpr = os.path.join(folder, "sky.xpr")

    print("verifying extractor against the faces that survive:")
    if not verify(xpr, folder):
        raise SystemExit("extractor does not reproduce known faces -- not writing anything")

    out = os.path.join(folder, "SkyYP.bmp")
    open(out, "wb").write(to_bmp24(unswizzle_face(face_bytes(xpr, "YP"))))
    print("wrote", out, os.path.getsize(out), "bytes")
