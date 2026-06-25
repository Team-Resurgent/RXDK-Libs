#!/usr/bin/env python3
"""Fix pack.h inserted between #ifndef and #define."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATTERN = re.compile(
    r'^(?P<ifndef>#ifndef[^\n]+)\n#include "pack.h"\n(?P<define>#define[^\n]+)',
    re.MULTILINE,
)


def main() -> int:
    roots = [ROOT / "libs/libxapi", ROOT / "include/sdk"]
    for root in roots:
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix not in {".h", ".c", ".cpp"}:
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            new_text, count = PATTERN.subn(
                r'\g<ifndef>\n\g<define>\n#include "pack.h"',
                text,
            )
            if count:
                path.write_text(new_text, encoding="utf-8")
                print(f"fixed {path.relative_to(ROOT)} ({count})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
