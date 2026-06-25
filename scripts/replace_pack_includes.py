#!/usr/bin/env python3
"""Replace pshpack/poppack includes with internal/pack.h macros."""

from __future__ import annotations

import importlib.util
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("trace", ROOT / "scripts/trace_xapi_includes.py")
trace = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(trace)

PACK_MAP = {
    "pshpack1": "RXDK_PACK_PUSH1",
    "pshpack2": "RXDK_PACK_PUSH2",
    "pshpack4": "RXDK_PACK_PUSH4",
    "pshpack8": "RXDK_PACK_PUSH8",
}
INC_RE = re.compile(r'^\s*#\s*include\s+([<"])([^>"]+)[>"]\s*$', re.I)


def main() -> int:
    reachable = trace.compute_all_reachable()
    extra = [ROOT / "include/sdk/wingdi.h", ROOT / "include/sdk/usb100.h"]
    paths = set(reachable) | set(extra)

    for path in sorted(paths):
        if not path.is_file() or path.suffix not in {".h", ".c", ".cpp"}:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        lines = text.splitlines()
        out: list[str] = []
        needs_pack_h = False
        changed = False
        for line in lines:
            match = INC_RE.match(line)
            if match:
                stem = Path(match.group(2)).stem.lower()
                if stem in PACK_MAP:
                    out.append(PACK_MAP[stem])
                    needs_pack_h = True
                    changed = True
                    continue
                if "poppack" in match.group(2).lower():
                    out.append("RXDK_PACK_POP")
                    needs_pack_h = True
                    changed = True
                    continue
            out.append(line)
        if not changed:
            continue
        if needs_pack_h and not any("pack.h" in line for line in out):
            insert_at = 0
            for index, line in enumerate(out):
                if line.startswith("#ifndef") or line.startswith("#if "):
                    insert_at = index + 1
                    break
                if line.startswith("#include"):
                    insert_at = index
                    break
            out.insert(insert_at, '#include "pack.h"')
        path.write_text("\n".join(out) + ("\n" if text.endswith("\n") else ""), encoding="utf-8")
        print(f"updated {path.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
