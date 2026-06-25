#!/usr/bin/env python3
"""Replace file-level #ifndef guards with #pragma once, keeping #define GUARD.

The #define is retained when other headers test the guard macro (e.g. NT_INCLUDED).
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIBXAPI = ROOT / "libs" / "libxapi"

RE_IFNDEF = re.compile(r"^#ifndef\s+(\w+)\s*$")
RE_DEFINE = re.compile(r"^#define\s+(\w+)\s*$")
RE_ENDIF = re.compile(r"^#endif\b(.*)$")


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


def find_guard(lines: list[str]) -> tuple[str, int] | None:
    i = skip_leading(lines, 0)
    if i + 1 >= len(lines):
        return None
    m_if = RE_IFNDEF.match(strip_line(lines[i]))
    m_def = RE_DEFINE.match(strip_line(lines[i + 1]))
    if not m_if or not m_def or m_if.group(1) != m_def.group(1):
        return None
    return m_if.group(1), i


def endif_names_guard(endif_line: str, guard: str) -> bool:
    m = RE_ENDIF.match(strip_line(endif_line))
    if not m:
        return False
    tail = m.group(1).strip()
    if not tail:
        return False
    if tail.startswith("//"):
        return guard in tail
    if tail.startswith("/*"):
        return guard in tail
    return False


def find_closing_endif(lines: list[str], guard: str) -> int | None:
    candidates: list[int] = []
    for i, line in enumerate(lines):
        if endif_names_guard(line, guard):
            candidates.append(i)
    if not candidates:
        return None
    close_idx = candidates[-1]
    for j in range(close_idx + 1, len(lines)):
        if not is_blank(lines[j]):
            return None
    return close_idx


def has_pragma_once_block(lines: list[str], before: int) -> bool:
    for i in range(min(before, len(lines))):
        if strip_line(lines[i]).strip() == "#pragma once":
            return True
    return False


def remove_msvc_pragma_once_block(lines: list[str], start: int) -> int:
    i = start
    n = len(lines)
    while i < n and is_blank(lines[i]):
        i += 1
    if i + 2 >= n:
        return start
    block = [strip_line(lines[i]).strip(), strip_line(lines[i + 1]).strip(), strip_line(lines[i + 2]).strip()]
    if block[0].startswith("#if") and "_MSC_VER" in block[0] and block[1] == "#pragma once" and block[2] == "#endif":
        j = i + 3
        while j < n and is_blank(lines[j]):
            j += 1
        return j
    return start


def convert_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8", errors="surrogateescape")
    newline = "\r\n" if "\r\n" in text else "\n"
    lines = text.splitlines(keepends=True)
    if not lines:
        return False

    found = find_guard(lines)
    if not found:
        return False

    guard, guard_idx = found
    close_idx = find_closing_endif(lines, guard)
    if close_idx is None or close_idx <= guard_idx:
        return False

    define_line = lines[guard_idx + 1]
    already = has_pragma_once_block(lines, guard_idx)

    new_lines: list[str] = []
    new_lines.extend(lines[:guard_idx])
    if not already:
        if new_lines and not is_blank(new_lines[-1]):
            new_lines.append(newline)
        new_lines.append("#pragma once" + newline)
    new_lines.append(define_line)
    if guard_idx + 2 < len(lines) and not is_blank(lines[guard_idx + 2]):
        if is_blank(new_lines[-1]):
            pass
        else:
            new_lines.append(newline)

    body_start = remove_msvc_pragma_once_block(lines, guard_idx + 2)
    new_lines.extend(lines[body_start:close_idx])

    tail_start = close_idx + 1
    while tail_start < len(lines) and is_blank(lines[tail_start]):
        tail_start += 1
    new_lines.extend(lines[tail_start:])

    new_text = "".join(new_lines)
    if new_text != text:
        path.write_text(new_text, encoding="utf-8", errors="surrogateescape")
        return True
    return False


def main() -> int:
    target = LIBXAPI
    if len(sys.argv) > 1:
        target = Path(sys.argv[1])

    changed: list[str] = []
    for path in sorted(target.rglob("*.h")):
        if convert_file(path):
            changed.append(str(path.relative_to(ROOT)))

    print(f"converted: {len(changed)}")
    for p in changed:
        print(f"  {p}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
