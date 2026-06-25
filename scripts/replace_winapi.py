#!/usr/bin/env python3
"""Replace standalone WINAPI tokens with __attribute__((__stdcall__)) in libxapi."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIBXAPI = ROOT / "libs" / "libxapi"

WINAPI = re.compile(r"\bWINAPI\b")
REPLACEMENT = "__attribute__((__stdcall__))"

SKIP_LINE = re.compile(
    r"^\s*#(?:define|ifndef|ifdef|undef|if)\s+.*\bWINAPI\b",
    re.IGNORECASE,
)

REMOVE_WINAPI_DEF = re.compile(
    r"\n#ifndef WINAPI\n#define WINAPI __stdcall\n#endif\n",
    re.MULTILINE,
)


def convert_lines(lines: list[str]) -> list[str]:
    out: list[str] = []
    for line in lines:
        if SKIP_LINE.match(line):
            out.append(line)
            continue
        out.append(WINAPI.sub(REPLACEMENT, line))
    return out


def convert_text(text: str) -> str:
    lines = text.splitlines(keepends=True)
    updated = "".join(convert_lines(lines))
    updated = REMOVE_WINAPI_DEF.sub("\n", updated)
    return updated


def patch_windef(text: str) -> str:
    text = re.sub(r"^#define WINAPI\s+.*$", "", text, flags=re.MULTILINE)
    text = re.sub(
        r"^#define APIENTRY\s+WINAPI\s*$",
        "#define APIENTRY    __attribute__((__stdcall__))",
        text,
        flags=re.MULTILINE,
    )
    return text


def patch_wtypes(text: str) -> str:
    return text.replace(
        "#ifndef WINAPI          // If not included with 3.1 headers...\n"
        "#define FAR             _far\n"
        "#define PASCAL          _pascal\n"
        "#define CDECL           _cdecl\n"
        "#define VOID            void\n"
        "#define WINAPI      FAR PASCAL\n"
        "#define CALLBACK    FAR PASCAL\n",
        "#if 0 /* Win16 calling-convention shims — not used on Xbox */\n"
        "#define FAR             _far\n"
        "#define PASCAL          _pascal\n"
        "#define CDECL           _cdecl\n"
        "#define VOID            void\n"
        "#define CALLBACK    FAR PASCAL\n",
    )


def main() -> int:
    exts = {".c", ".cpp", ".h", ".hpp"}
    changed: list[str] = []
    for path in sorted(LIBXAPI.rglob("*")):
        if path.suffix.lower() not in exts:
            continue
        original = path.read_text(encoding="utf-8", errors="surrogateescape")
        updated = convert_text(original)
        if path.name == "windef.h":
            updated = patch_windef(updated)
        if path.name == "wtypes.h":
            updated = patch_wtypes(updated)
        if updated != original:
            path.write_text(updated, encoding="utf-8", errors="surrogateescape")
            changed.append(str(path.relative_to(ROOT)))

    site = ROOT / "build" / "generated" / "xapi_site.h"
    if site.exists():
        site_text = site.read_text(encoding="utf-8")
        site_new = re.sub(
            r"/\* Clang x86-windows-gnu leaves windef\.h WINAPI empty; xAPI is stdcall throughout\. \*/\n"
            r"#ifdef __clang__\n"
            r"#undef WINAPI\n"
            r"#define WINAPI __stdcall\n"
            r"#undef CALLBACK\n"
            r"#define CALLBACK __stdcall\n"
            r"#endif\n\n",
            "",
            site_text,
        )
        if site_new != site_text:
            site.write_text(site_new, encoding="utf-8")
            changed.append(str(site.relative_to(ROOT)))

    print(f"updated: {len(changed)}")
    for p in changed:
        print(f"  {p}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
