#!/usr/bin/env python3
"""Insert missing #define GUARD lines after #pragma once (post-conversion repair)."""

from __future__ import annotations

import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIBXAPI = ROOT / "libs" / "libxapi"

RE_IFNDEF = re.compile(r"^#ifndef\s+(\w+)\s*$")
RE_DEFINE = re.compile(r"^#define\s+(\w+)\s*$")


def strip_line(s: str) -> str:
    return s.rstrip("\n\r")


def is_blank(line: str) -> bool:
    return strip_line(line).strip() == ""


def skip_leading(lines: list[str], start: int) -> int:
    i = start
    n = len(lines)
    while i < n:
        s = strip_line(lines[i]).strip()
        if not s:
            i += 1
            continue
        if s.startswith("//"):
            i += 1
            continue
        if s.startswith("/*"):
            if s.endswith("*/") and s.count("/*") == 1:
                i += 1
                continue
            i += 1
            while i < n:
                if "*/" in strip_line(lines[i]):
                    i += 1
                    break
                i += 1
            continue
        break
    return i


def guard_from_head(rel: str) -> str | None:
    try:
        text = subprocess.check_output(
            ["git", "show", f"HEAD:{rel.replace(chr(92), '/')}"],
            cwd=ROOT,
            stderr=subprocess.DEVNULL,
        ).decode("utf-8", errors="replace")
    except subprocess.CalledProcessError:
        return None
    lines = text.splitlines()
    i = skip_leading(lines, 0)
    if i + 1 >= len(lines):
        return None
    m_if = RE_IFNDEF.match(lines[i].strip())
    m_def = RE_DEFINE.match(lines[i + 1].strip())
    if not m_if or not m_def or m_if.group(1) != m_def.group(1):
        return None
    return m_if.group(1)


def repair_file(path: Path) -> bool:
    rel = str(path.relative_to(ROOT)).replace("\\", "/")
    text = path.read_text(encoding="utf-8", errors="surrogateescape")
    newline = "\r\n" if "\r\n" in text else "\n"
    lines = text.splitlines(keepends=True)
    i = skip_leading(lines, 0)
    if i >= len(lines) or strip_line(lines[i]).strip() != "#pragma once":
        return False
    j = i + 1
    while j < len(lines) and is_blank(lines[j]):
        j += 1
    if j < len(lines):
        m_def = RE_DEFINE.match(strip_line(lines[j]))
        if m_def:
            return False

    guard = guard_from_head(rel)
    if guard is None:
        return False

    insert = f"#define {guard}" + newline
    new_lines = lines[: i + 1] + [insert, newline] + lines[i + 1 :]
    new_text = "".join(new_lines)
    if new_text != text:
        path.write_text(new_text, encoding="utf-8", errors="surrogateescape")
        return True
    return False


def main() -> int:
    fixed: list[str] = []
    for path in sorted(LIBXAPI.rglob("*.h")):
        if repair_file(path):
            fixed.append(str(path.relative_to(ROOT)))
    print(f"repaired: {len(fixed)}")
    for p in fixed:
        print(f"  {p}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
