#!/usr/bin/env python3
"""Compile-only C header availability check (stdc header manifest, RXDK include paths).

Manifest is tools/conformance_headers.txt (vendored from winspool/stdtests). Host-side
substitute for an autotools configure on Windows.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tools" / "conformance_headers.txt"
ZIG = "zig"

INCLUDE_DIRS = [
    ROOT / "include",
    ROOT / "build" / "generated",
    ROOT / "src" / "runtime" / "c23",
    ROOT / "vendor" / "picolibc" / "libc" / "include",
    ROOT / "vendor" / "picolibc" / "libc" / "machine" / "x86",
]

# RXDK kit profile: headers we do not expect to ship yet (informational skip).
SKIP_HEADERS = frozenset(
    {
        "threads.h",
        "signal.h",
        "complex.h",
        "fenv.h",
        "tgmath.h",
        "stdatomic.h",
        "uchar.h",
        "wchar.h",
        "wctype.h",
        "locale.h",
        "stdmchar.h",
        "stdcountof.h",
        "iso646.h",
        "vararg.h",
    }
)

COMPILE_FLAGS = [
    "-target",
    "x86-windows-gnu",
    "-std=c23",
    "-ffreestanding",
    "-fno-stack-protector",
    "-fno-sanitize=undefined",
    "-c",
]


def parse_manifest(path: Path) -> list[str]:
    headers: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        m = re.match(r"\[([^\]]+\.h)\]", line.strip())
        if m:
            headers.append(m.group(1))
    return headers


def try_compile(header: str, work: Path) -> tuple[str, str]:
    if header in SKIP_HEADERS:
        return "skip", "rxdk profile"
    src = work / f"check_{header.replace('/', '_')}.c"
    src.write_text(
        f'#include <{header}>\nint rxdk_header_check(void) {{ return 0; }}\n',
        encoding="utf-8",
        newline="\n",
    )
    out = work / f"{src.stem}.o"
    cmd = [ZIG, "cc", *COMPILE_FLAGS, "-o", str(out), str(src)]
    for inc in INCLUDE_DIRS:
        cmd.extend(["-I", str(inc)])
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if proc.returncode == 0:
        return "ok", ""
    err = (proc.stderr or proc.stdout or "").strip().splitlines()
    summary = err[-1] if err else f"exit {proc.returncode}"
    return "fail", summary


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--manifest", type=Path, default=MANIFEST)
    ap.add_argument("--report", type=Path, help="Write TAP-ish summary to this file")
    args = ap.parse_args()

    if not args.manifest.is_file():
        print(f"error: manifest not found: {args.manifest}", file=sys.stderr)
        print("Expected tools/conformance_headers.txt", file=sys.stderr)
        return 2

    headers = parse_manifest(args.manifest)
    results: list[tuple[str, str, str]] = []
    with tempfile.TemporaryDirectory(prefix="rxdk-headers-") as tmp:
        work = Path(tmp)
        for h in headers:
            status, detail = try_compile(h, work)
            results.append((h, status, detail))
            mark = {"ok": "ok", "fail": "not ok", "skip": "ok # SKIP"}[status]
            line = f"{mark} - {h}"
            if detail and status != "ok":
                line += f" ({detail})"
            print(line)

    ok = sum(1 for _, s, _ in results if s == "ok")
    fail = sum(1 for _, s, _ in results if s == "fail")
    skip = sum(1 for _, s, _ in results if s == "skip")
    print(f"# passed={ok} failed={fail} skipped={skip} total={len(results)}")

    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        lines = [f"{h}\t{s}\t{d}" for h, s, d in results]
        args.report.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
        print(f"wrote {args.report.relative_to(ROOT)}")

    return 1 if fail else 0


if __name__ == "__main__":
    raise SystemExit(main())
