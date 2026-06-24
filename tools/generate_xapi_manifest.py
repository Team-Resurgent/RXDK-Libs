#!/usr/bin/env python3
"""Parse NT sources files and emit build/xapi_sources.zig for build/xapi.zig."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "vendor" / "xbox_private"
OUT = ROOT / "build" / "xapi_sources.zig"

SLICES: list[tuple[str, str, str, bool]] = [
    ("k32", "private/ntos/xapi/k32/lib/sources", "private/ntos/xapi/k32", False),
    ("dll", "private/ntos/xapi/dll/sources", "private/ntos/xapi/dll", False),
    ("rtl", "private/ntos/rtl/xapi/sources", "private/ntos/rtl/xapi", False),
    ("uuid", "private/genx/types/uuid/sources", "private/genx/types/uuid", False),
    ("ohcd", "private/ntos/dd/usb/ohcd/sources", "private/ntos/dd/usb/ohcd", False),
    ("usbd", "private/ntos/dd/usb/usbd/sources", "private/ntos/dd/usb/usbd", True),
    ("usbhub", "private/ntos/dd/usb/usbhub/sources", "private/ntos/dd/usb/usbhub", True),
    ("mu", "private/ntos/dd/usb/mu/sources", "private/ntos/dd/usb/mu", True),
    ("xid", "private/ntos/dd/usb/xid/sources", "private/ntos/dd/usb/xid", True),
]

K32_EXTRA = [
    "build/xapi_intrlock.c",
    "build/xapi_fiber_switch.S",
    "build/xapi_muldiv.c",
    "build/xapi_rip.c",
    "build/xapi_compat.c",
    "build/xapi_kernel_patches.S",
    "build/xapi_findfirstset.c",
    "build/xapi_tls_array.c",
    "build/xapi_lasterror.c",
    "build/xapi_tls.c",
    "build/xapi_tls_data.c",
    "build/xapi_fiber_tls.c",
    "build/xapi_fiber_tls_data.c",
]
K32_SKIP_SUFFIXES = (
    "/i386/intrlock.asm",
    "/intrlock.asm",
    "/i386/muldiv.asm",
    "/muldiv.asm",
    "/k32/tls.c",
)
DLL_EXTRA = [
    "build/xapi_tls_array.c",
]
DLL_SKIP_SUFFIXES = (
    "/xapiatls.asm",
    "/i386/xapiatls.asm",
)


def merge_relative_paths(tokens: list[str]) -> list[str]:
    out: list[str] = []
    i = 0
    while i < len(tokens):
        if tokens[i] == ".." and i + 1 < len(tokens):
            out.append(f"../{tokens[i + 1]}")
            i += 2
        elif tokens[i] == ".." and i + 2 < len(tokens):
            out.append(f"../{tokens[i + 1]}/{tokens[i + 2]}")
            i += 3
        else:
            out.append(tokens[i])
            i += 1
    return out


def parse_sources(path: Path) -> tuple[list[str], list[str]]:
    text = path.read_text(encoding="utf-8", errors="replace")
    c_sources: list[str] = []
    asm_sources: list[str] = []
    section = "c"
    buf = ""
    for raw in text.splitlines():
        line = raw.split("!", 1)[0].strip()
        if not line:
            continue
        if line.startswith("NTTARGETFILES"):
            continue
        if line.startswith("SOURCES="):
            section = "c"
            buf = ""
            line = line[len("SOURCES=") :].strip()
        elif line.startswith("i386_SOURCES="):
            section = "asm"
            buf = ""
            line = line[len("i386_SOURCES=") :].strip()
        elif line.startswith("SOURCES") or line.startswith("i386_SOURCES"):
            continue
        buf += " " + line
        if not line.rstrip().endswith("\\"):
            body = buf.strip()
            body = re.sub(r"\\(?=\s)", "", body)
            body = body.replace("\\", "/")
            tokens: list[str] = []
            for token in re.split(r"\s+", body):
                if not token:
                    continue
                tokens.append(token)
            buf = ""
            merged = merge_relative_paths(tokens)
            for token in merged:
                low = token.lower()
                if low.endswith(".asm"):
                    asm_sources.append(token)
                elif low.endswith((".c", ".cpp")):
                    c_sources.append(token)
    return c_sources, asm_sources


def vendor_path(cwd: str, name: str) -> str:
    name = name.replace("\\", "/")
    base = cwd
    # k32/lib/sources uses ..\i386\*.asm relative to the lib/ subdir.
    if name.startswith("../") and cwd.endswith("/xapi/k32"):
        base = f"{cwd}/lib"
    parts: list[str] = []
    for part in (Path(base) / name).as_posix().split("/"):
        if part == "..":
            if parts:
                parts.pop()
        elif part and part != ".":
            parts.append(part)
    return f"vendor/xbox_private/{'/'.join(parts)}"


def normalize_asm_path(name: str) -> str:
    name = name.replace("\\", "/")
    if name.startswith("../") or "/" in name:
        return name
    if name.endswith(".asm"):
        return f"i386/{name}"
    return name


def collect_slice(name: str, sources_rel: str, cwd: str, is_cpp: bool) -> list[str]:
    path = VENDOR / sources_rel.replace("/", "\\")
    if not path.is_file():
        print(f"warning: missing {path}", file=sys.stderr)
        return []
    c_list, asm_list = parse_sources(path)
    files: list[str] = []
    for s in c_list:
        files.append(vendor_path(cwd, s))
    for s in asm_list:
        files.append(vendor_path(cwd, normalize_asm_path(s)))
    if name == "k32":
        for extra in K32_EXTRA:
            files.append(extra if extra.startswith("build/") else f"vendor/xbox_private/{extra}")
        files = [f for f in files if not f.endswith("/xsndtrk.c")]
        files = [f for f in files if not any(f.endswith(s) for s in K32_SKIP_SUFFIXES)]
    elif name == "dll":
        for extra in DLL_EXTRA:
            files.append(extra if extra.startswith("build/") else f"vendor/xbox_private/{extra}")
        files = [f for f in files if not any(f.endswith(s) for s in DLL_SKIP_SUFFIXES)]
    # Dedupe, stable order
    seen: set[str] = set()
    out: list[str] = []
    for f in files:
        if f not in seen:
            seen.add(f)
            out.append(f)
    return out


def main() -> int:
    if not VENDOR.is_dir():
        print(f"error: run scripts/sync-xapi-vendor.ps1 first ({VENDOR})", file=sys.stderr)
        return 2

    slice_files: dict[str, list[str]] = {}
    slice_cpp: dict[str, bool] = {}
    for name, sources_rel, cwd, is_cpp in SLICES:
        slice_files[name] = collect_slice(name, sources_rel, cwd, is_cpp)
        slice_cpp[name] = is_cpp

    lines: list[str] = [
        "// Generated by tools/generate_xapi_manifest.py — do not edit.",
        "",
        "pub const Slice = struct {",
        "    name: []const u8,",
        "    sources: []const []const u8,",
        "    is_cpp: bool,",
        "};",
        "",
    ]
    for name, files in slice_files.items():
        lines.append(f"pub const {name}_sources = [_][]const u8{{")
        for f in files:
            lines.append(f'    "{f}",')
        lines.append("};")
        lines.append("")

    lines.append("pub const slices = [_]Slice{")
    for name in slice_files:
        cpp = "true" if slice_cpp[name] else "false"
        lines.append(f"    .{{ .name = \"{name}\", .is_cpp = {cpp}, .sources = &{name}_sources }}, ")
    lines.append("};")
    lines.append("")

    OUT.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    total = sum(len(v) for v in slice_files.values())
    print(f"wrote {OUT.relative_to(ROOT)} ({total} sources across {len(slice_files)} slices)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
