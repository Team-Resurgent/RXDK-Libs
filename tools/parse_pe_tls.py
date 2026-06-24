#!/usr/bin/env python3
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "zig-out/samples/xapi-smoke/xapi-smoke.exe"


def main() -> int:
    pe = EXE.read_bytes()
    e = struct.unpack_from("<I", pe, 0x3C)[0]
    opt = e + 24
    base = struct.unpack_from("<I", pe, opt + 28)[0]
    nsec = struct.unpack_from("<H", pe, e + 6)[0]
    opt_size = struct.unpack_from("<H", pe, e + 20)[0]
    sec_off = e + 24 + opt_size
    print(f"{EXE.name} base={base:#x} sections={nsec}")
    for i in range(nsec):
        o = sec_off + i * 40
        name = pe[o : o + 8].split(b"\0")[0].decode(errors="replace")
        vsz = struct.unpack_from("<I", pe, o + 8)[0]
        va = struct.unpack_from("<I", pe, o + 12)[0]
        raw = struct.unpack_from("<I", pe, o + 20)[0]
        if "tls" in name.lower():
            print(f"  {name:8} va={va:#x} vsize={vsz} raw={raw:#x}")
            if raw and vsz:
                chunk = pe[raw : raw + min(vsz, 64)]
                print(f"    data: {chunk.hex()}")
    off = sec_off
    tls_rva = struct.unpack_from("<I", pe, opt + 0xC0)[0]
    if tls_rva:
        for i in range(nsec):
            o = sec_off + i * 40
            va = struct.unpack_from("<I", pe, o + 12)[0]
            raw = struct.unpack_from("<I", pe, o + 20)[0]
            vs = struct.unpack_from("<I", pe, o + 8)[0]
            if va <= tls_rva < va + vs:
                off = raw + tls_rva - va
                break
        start = struct.unpack_from("<I", pe, off)[0]
        end = struct.unpack_from("<I", pe, off + 4)[0]
        zf = struct.unpack_from("<I", pe, off + 12)[0]
        print(f"TLS directory: start={start:#x} end={end:#x} raw={end-start} zerofill={zf}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
