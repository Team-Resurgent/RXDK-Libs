#!/usr/bin/env python3
import struct
import sys


def coff_section_names(path: str) -> None:
    d = open(path, "rb").read()
    nsec = struct.unpack_from("<H", d, 2)[0]
    opt = struct.unpack_from("<H", d, 16)[0]
    so = 20 + opt
    str_off = so + nsec * 40

    def name(raw: bytes) -> str:
        if raw[0:1] == b"/":
            digits = b""
            for c in raw[1:8]:
                if 48 <= c <= 57:
                    digits += bytes([c])
                else:
                    break
            off = int(digits.decode())
            end = d.find(b"\0", str_off + off)
            return d[str_off + off : end].decode("ascii", "replace")
        return raw.split(b"\0")[0].decode("ascii", "replace")

    print(path)
    for i in range(nsec):
        o = so + i * 40
        nm = name(d[o : o + 8])
        size = struct.unpack_from("<I", d, o + 16)[0]
        if "tls" in nm.lower() or size in (4, 16, 256):
            print(f"  {nm:48} {size:5}")


if __name__ == "__main__":
    for p in sys.argv[1:]:
        coff_section_names(p)
