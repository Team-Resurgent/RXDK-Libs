#!/usr/bin/env python3
"""Replace standalone FASTCALL tokens with __attribute__((fastcall)) in libxapi."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIBXAPI = ROOT / "libs" / "libxapi"

FASTCALL = re.compile(r"\bFASTCALL\b")
REPLACEMENT = "__attribute__((fastcall))"

DEFINE_BLOCK = re.compile(
    r"\n#if defined\(_M_IX86\)\n#define FASTCALL __fastcall\n#else\n#define FASTCALL\n#endif\n",
    re.MULTILINE,
)

WINEF_COMPAT_BLOCK = re.compile(
    r"\n#ifndef FASTCALL\n#define FASTCALL __attribute__\(\(fastcall\)\)\n#endif\n",
    re.MULTILINE,
)


def convert_text(text: str) -> str:
    text = DEFINE_BLOCK.sub("\n", text)
    text = WINEF_COMPAT_BLOCK.sub("\n", text)
    return FASTCALL.sub(REPLACEMENT, text)


def main() -> int:
    exts = {".c", ".cpp", ".h", ".hpp"}
    changed: list[str] = []
    for path in sorted(LIBXAPI.rglob("*")):
        if path.suffix.lower() not in exts:
            continue
        if path.name == "windef_compat.h":
            continue
        original = path.read_text(encoding="utf-8", errors="surrogateescape")
        updated = convert_text(original)
        if updated != original:
            path.write_text(updated, encoding="utf-8", errors="surrogateescape")
            changed.append(str(path.relative_to(ROOT)))
    print(f"updated: {len(changed)}")
    for p in changed:
        print(f"  {p}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
