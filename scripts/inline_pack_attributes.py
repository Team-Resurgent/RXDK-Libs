#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPLS = [
    ("RXDK_ALIGN8", "__attribute__((aligned(8)))"),
    ("RXDK_PACK2", "__attribute__((packed, aligned(2)))"),
    ("RXDK_PACKED", "__attribute__((packed))"),
]

for root in (ROOT / "libs/libxapi", ROOT / "include/sdk"):
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in {".h", ".c", ".cpp"}:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        orig = text
        lines = [line for line in text.splitlines() if line.strip() != '#include "pack.h"']
        text = "\n".join(lines)
        if orig.endswith("\n"):
            text += "\n"
        for old, new in REPLS:
            text = text.replace(old, new)
        if text != orig:
            path.write_text(text, encoding="utf-8")
            print(path.relative_to(ROOT))
