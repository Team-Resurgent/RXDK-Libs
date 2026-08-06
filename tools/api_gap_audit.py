"""Public 5849 APIs we do not implement.

The constants sweep answers "does this symbol have a value"; this answers the
harder question, "does this function exist at all". A title that calls a missing
API fails to link, which is a louder failure than a missing constant but easier
to miss during development, because nothing in our sample suite calls it.

A name counts as a public API only if it is BOTH declared in a 5849 public header
(with WINAPI/STDAPI/__stdcall/APIENTRY, the shape an exported function has) AND
defined in a shipped 5849 library. That intersection matters: the headers alone
carry macros and inline helpers that were never exported, and the libraries alone
carry a great deal of internal code that was never public -- 5849's xvoice.lib,
for instance, contains the binary-only codec, which is not a gap in our port so
much as something we were never going to have.

Names are compared with the @n stdcall suffix stripped, because here the question
is which function, not which calling convention. (The ownership audit does the
opposite, deliberately -- see the note in lib_ownership_audit.py.)

    python tools/api_gap_audit.py
"""

import glob
import os
import re
import struct


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


XDK_LIB = r"D:\Git\RXDK\POC\XDKSetup5849.17\XDK\xbox\lib"
XDK_INC = r"D:\Git\RXDK\POC\XDKSetup5849.17\XDK\xbox\include"
# NOTE: this reads dist/, which is only refreshed by build-iso.ps1 -Dist.
# A freshly implemented API stays on the "missing" list until you rebuild it --
# IDirectSoundBuffer_Pause reported missing for exactly that reason, hours after
# being written. Rebuild dist before believing a result.
OUR_LIB = r"D:\Git\RXDK-Libs\dist\lib\debug"
OUR_INC = r"D:\Git\RXDK-Libs\shared\include"


def archive_symbols(path):
    """Defined symbols of a COFF archive, from its first linker member."""
    with open(path, 'rb') as f:
        data = f.read()
    if data[:8] != b'!<arch>\n':
        return set()
    hdr = data[8:68]
    size = int(hdr[48:58].decode('ascii', 'replace').strip() or 0)
    body = data[68:68 + size]
    if len(body) < 4:
        return set()
    count = struct.unpack('>I', body[:4])[0]
    names = body[4 + 4 * count:]
    out, cur = set(), bytearray()
    for b in names:
        if b == 0:
            if cur:
                out.add(cur.decode('ascii', 'replace'))
            cur = bytearray()
        else:
            cur.append(b)
    return out


def base(sym):
    """Strip a leading underscore and any @n stdcall suffix."""
    if sym.startswith('_') and not sym.startswith('__'):
        sym = sym[1:]
    if '@' in sym:
        head, _, tail = sym.rpartition('@')
        if tail.isdigit():
            return head
    return sym


def declared(path):
    """Exported-function names declared in a header."""
    try:
        text = open(path, encoding='utf-8', errors='replace').read()
    except OSError:
        return set()
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    text = re.sub(r'//[^\n]*', ' ', text)
    return {m.group(1) for m in re.finditer(
        r'(?:WINAPI|STDAPI|__stdcall|APIENTRY)\s+\**([A-Za-z_]\w+)\s*\(', text)}


def main():
    xdk = set()
    for p in glob.glob(os.path.join(XDK_LIB, '*.lib')):
        b = os.path.basename(p).lower()[:-4]
        # Debug/instrumented/LTCG variants are not authoritative. Match the whole
        # suffix against the RETAIL base name -- a bare endswith('d') silently
        # excluded dsound.lib, which simply ends in a 'd', and with it every
        # DirectSound API this audit exists to find.
        if is_variant(b):
            continue
        xdk |= {base(s) for s in archive_symbols(p)}

    ours = set()
    for p in glob.glob(os.path.join(OUR_LIB, '*.lib')):
        ours |= {base(s) for s in archive_symbols(p)}

    rows = []
    for hp in sorted(glob.glob(os.path.join(XDK_INC, '*.h'))):
        name = os.path.basename(hp)
        if not os.path.exists(os.path.join(OUR_INC, name)):
            continue
        public = {n for n in declared(hp) if n in xdk}
        missing = sorted(n for n in public if n not in ours)
        if missing:
            rows.append((name, len(public), missing))

    rows.sort(key=lambda r: -len(r[2]))
    print(f"public 5849 APIs absent from our libs: {sum(len(r[2]) for r in rows)}\n")
    for name, total, missing in rows:
        print(f"{name:18s} {len(missing):3d} missing of {total}")
        for m in missing:
            print(f"    {m}")
        print()


if __name__ == '__main__':
    main()
