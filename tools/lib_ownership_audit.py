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
    """Strip decoration so an MSVC name and ours compare: leading _, @n suffix."""
    s = sym
    if s.startswith('_') and not s.startswith('__'):
        s = s[1:]
    if '@' in s and s.split('@')[-1].isdigit():
        s = s.rsplit('@',1)[0]
    return s

XDK = r"D:\Git\RXDK\POC\XDKSetup5849.17\XDK\xbox\lib"
OURS = r"D:\Git\RXDK-Libs\dist\lib\debug"

# 5849 retail libs only: skip debug (d), instrumented (i), LTCG.
xdk = {}
for p in glob.glob(os.path.join(XDK, '*.lib')):
    b = os.path.basename(p).lower()[:-4]
    if b.endswith('ltcg') or b.endswith('d') or b.endswith('i'):
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
 'libxonline':'xonline','libxact':'xact','libxvoice':'xvoice','libdmusic':'dmusic',
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
