#!/usr/bin/env python3
"""Replace RXDK_PACK_PUSH/POP regions with per-struct attribute tags."""

from __future__ import annotations

import importlib.util
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("trace", ROOT / "scripts/trace_xapi_includes.py")
trace = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(trace)

ATTR_BY_LEVEL = {
    1: "RXDK_PACKED",
    2: "RXDK_PACK2",
    4: None,  # x86 default layout matches MSVC #pragma pack(4)
    8: "RXDK_ALIGN8",
}

STRUCT_CLOSE_RE = re.compile(
    r"^(\})\s*([A-Za-z_][A-Za-z0-9_]*(?:\s*,\s*\*[A-Za-z_][A-Za-z0-9_]*)?)\s*;\s*$"
)
PUSH_RE = re.compile(r"^RXDK_PACK_PUSH(\d+)\s*(?://.*)?$")
POP_RE = re.compile(r"^RXDK_PACK_POP\s*(?://.*)?$")
PACK_H_RE = re.compile(r'^#include "pack.h"\s*$')


def tag_struct_closures(lines: list[str], attr: str | None) -> list[str]:
    if attr is None:
        return lines
    out: list[str] = []
    for line in lines:
        match = STRUCT_CLOSE_RE.match(line)
        if match and attr not in line:
            out.append(f"{match.group(1)} {attr} {match.group(2)};")
        else:
            out.append(line)
    return out


def convert_text(text: str) -> tuple[str, bool]:
    lines = text.splitlines()
    out: list[str] = []
    stack: list[int] = []
    region: list[str] = []
    changed = False
    needs_pack_h = "RXDK_PACKED" in text or "RXDK_PACK2" in text or "RXDK_ALIGN8" in text

    def flush_region() -> None:
        nonlocal changed, needs_pack_h
        if not region:
            return
        level = stack[-1]
        attr = ATTR_BY_LEVEL[level]
        converted = tag_struct_closures(region, attr)
        if converted != region:
            changed = True
        if attr is not None:
            needs_pack_h = True
        out.extend(converted)
        region.clear()

    for line in lines:
        stripped = line.strip()
        push = PUSH_RE.match(stripped)
        if push:
            stack.append(int(push.group(1)))
            changed = True
            continue
        if POP_RE.match(stripped):
            if stack:
                flush_region()
                stack.pop()
            changed = True
            continue
        if stack:
            region.append(line)
        else:
            out.append(line)

    if region:
        flush_region()

    if not changed:
        return text, False

    result_lines = out
    if needs_pack_h and not any(PACK_H_RE.match(line.strip()) for line in result_lines):
        insert_at = 0
        for index, line in enumerate(result_lines):
            if line.startswith("#ifndef"):
                insert_at = index + 1
                while insert_at < len(result_lines) and result_lines[insert_at].startswith("#define"):
                    insert_at += 1
                break
            if line.startswith("#include"):
                insert_at = index
                break
        result_lines.insert(insert_at, '#include "pack.h"')

    # Drop pack.h if no attribute macros remain used.
    joined = "\n".join(result_lines)
    if joined.endswith("\n") or text.endswith("\n"):
        joined += "\n"
    uses_attr = any(token in joined for token in ("RXDK_PACKED", "RXDK_PACK2", "RXDK_ALIGN8"))
    if not uses_attr:
        result_lines = [line for line in result_lines if not PACK_H_RE.match(line.strip())]
        joined = "\n".join(result_lines)
        if text.endswith("\n"):
            joined += "\n"
    return joined, True


def main() -> int:
    extra = [ROOT / "include/sdk/wingdi.h", ROOT / "include/sdk/usb100.h"]
    paths = set(trace.compute_all_reachable()) | set(extra)
    for path in sorted(paths):
        if not path.is_file() or path.suffix not in {".h", ".c", ".cpp"}:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if "RXDK_PACK_PUSH" not in text and "RXDK_PACK_POP" not in text:
            continue
        new_text, changed = convert_text(text)
        if changed:
            path.write_text(new_text, encoding="utf-8")
            print(f"converted {path.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
