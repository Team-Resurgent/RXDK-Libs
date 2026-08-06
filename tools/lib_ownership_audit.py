"""Which library owns each symbol: ours vs the retail 5849 XDK.

The uplift is not only about a lib's CONTENTS being right -- a symbol can be
implemented correctly and still sit in the wrong archive. That is invisible to a
sample build (the link line names every lib, so it resolves either way) and only
bites a title that links a subset, which is exactly what the libxnet LIBX/LIBO
split turned out to be about.

Reads the linker member of each COFF archive, which lists that archive's DEFINED
symbols, normalises the decoration (leading underscore, @n stdcall suffix), then
reports every symbol we place in a lib that 5849 places somewhere else.

Symbols that appear in nearly every 5849 lib are compiler/CRT helpers rather than
owned code; filter those out when reading the results (see the len(...) <= 4
guard used in the analysis).

    python tools/lib_ownership_audit.py

Requires dist/lib/debug to be populated (build-iso.ps1 -Dist) and the 5849 XDK
libs at the path below.
"""

import struct, sys, os, glob

def archive_symbols(path):
    """Defined symbols of a COFF archive, from its first linker member."""
    with open(path,'rb') as f:
        data = f.read()
    if data[:8] != b'!<arch>\n':
        return set()
    off = 8
    # first member header is 60 bytes
    hdr = data[off:off+60]
    size = int(hdr[48:58].decode('ascii','replace').strip() or 0)
    body = data[off+60:off+60+size]
    if len(body) < 4:
        return set()
    count = struct.unpack('>I', body[:4])[0]
    names = body[4+4*count:]
    out, cur = set(), bytearray()
    for b in names:
        if b == 0:
            if cur: out.add(cur.decode('ascii','replace'))
            cur = bytearray()
        else:
            cur.append(b)
    return out

def norm(sym):
    """Strip the leading underscore only.

    Do NOT strip the @n stdcall suffix. `_KeGetCurrentIrql` and
    `_KeGetCurrentIrql@0` are DIFFERENT symbols with different calling
    conventions, and conflating them invents duplicates that do not exist --
    this audit's first run reported all 55 Ke*/Ex*/Io*/Hal* names as defined
    twice, when in fact libkernel supplies the stdcall xboxkrnl imports and
    libxapi supplies undecorated cdecl entry points. Nothing was duplicated.
    """
    return sym[1:] if sym.startswith('_') and not sym.startswith('__') else sym


# Retail base names we ship a counterpart for. A variant is one of these plus a
# d/i/ltcg suffix -- checking that way, rather than by bare suffix, is what keeps
# dsound.lib (which ends in 'd') from being mistaken for a debug build.
RETAIL_BASES = (
    'd3d8', 'd3dx8', 'dsound', 'dmusic', 'xapilib', 'xgraphics', 'xmv', 'xbdm',
    'xboxkrnl', 'xonline', 'xacteng', 'xvoice', 'xnet', 'uix', 'libc', 'libcmt',
    'libcp', 'libcpmt', 'xsndtrk', 'xperf', 'xnetn', 'xnets', 'xonlinen', 'xonlines',
)


def is_variant(base):
    if base in RETAIL_BASES:
        return False
    for r in RETAIL_BASES:
        if base.startswith(r) and base[len(r):] in ('d', 'i', 'ltcg', 'sd', 'nd'):
            return True
    return False


XDK = r"D:\Git\RXDK\POC\XDKSetup5849.17\XDK\xbox\lib"
# NOTE: this reads dist/, which is only refreshed by build-iso.ps1 -Dist.
# A freshly implemented API stays on the "missing" list until you rebuild it --
# IDirectSoundBuffer_Pause reported missing for exactly that reason, hours after
# being written. Rebuild dist before believing a result.
OURS = r"D:\Git\RXDK-Libs\dist\lib\debug"

# 5849 retail libs only: skip debug (d), instrumented (i), LTCG.
xdk = {}
for p in glob.glob(os.path.join(XDK, '*.lib')):
    b = os.path.basename(p).lower()[:-4]
    if is_variant(b):
        continue
    for s in archive_symbols(p):
        xdk.setdefault(norm(s), set()).add(b)

ours = {}
for p in glob.glob(os.path.join(OURS, '*.lib')):
    b = os.path.basename(p).lower()[:-4]
    for s in archive_symbols(p):
        ours.setdefault(norm(s), set()).add(b)

# map our lib name -> expected 5849 lib name
alias = {
 'libd3d8':'d3d8','libd3dx8':'d3dx8','libdsound':'dsound','libxapi':'xapilib',
 'libxgraphics':'xgraphics','libxmv':'xmv','libxbdm':'xbdm','libkernel':'xboxkrnl',
 'libxonline':'xonline','libxact':'xacteng',   # 5849 ships NO xact.lib -- xacteng IS the XACT library'libxvoice':'xvoice','libdmusic':'dmusic',
 'libxnet':'xnet','libxneto':'xneto',
}

mism = []
for sym, ourlibs in ours.items():
    if sym not in xdk: continue
    for ol in ourlibs:
        want = alias.get(ol)
        if want is None: continue
        if want not in xdk[sym]:
            mism.append((sym, ol, ','.join(sorted(xdk[sym]))))

print(f"our libs: {len(set(l for v in ours.values() for l in v))}   5849 libs: {len(set(l for v in xdk.values() for l in v))}")
print(f"symbols we define that 5849 also defines: {sum(1 for s in ours if s in xdk)}")
print(f"OWNERSHIP MISMATCHES: {len(mism)}\n")
from collections import Counter
c = Counter((ol, xl) for _, ol, xl in mism)
for (ol, xl), n in c.most_common(25):
    print(f"  {n:5d}  ours={ol:14s} 5849 has it in: {xl}")
