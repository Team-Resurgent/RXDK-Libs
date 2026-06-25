#!/usr/bin/env python3
"""Compute reachable headers for libxapi compile closure."""

from __future__ import annotations

import re
import subprocess
import sys
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIB = ROOT / "libs" / "libxapi"
SOURCES_ZIG = LIB / "sources.zig"

INCLUDE_DIRS = [
    LIB / "xapi" / "internal",
    LIB / "xapi" / "site",
    LIB / "xapi" / "win32",
    LIB / "xapi" / "nt",
    LIB / "xapi" / "k32",
    LIB / "xapi" / "k32" / "inc",
    LIB / "xapi" / "dll",
    LIB / "xapi" / "rtl" / "inc",
    LIB / "xapi" / "rtl",
    LIB / "xapi" / "uuid",
    LIB / "xapi" / "usb" / "inc",
    LIB / "xapi" / "usb" / "ohcd",
    LIB / "xapi" / "usb" / "usbd",
    LIB / "xapi" / "usb" / "hub",
    LIB / "xapi" / "usb" / "mu",
    LIB / "xapi" / "usb" / "xid",
    LIB / "xapi" / "support" / "inc",
    LIB / "xapi" / "support" / "inc" / "ntos",
    LIB / "xapi" / "support" / "fatx",
    LIB / "xapi" / "support" / "idex",
    LIB / "xapi" / "support" / "crypto",
    LIB / "xapi" / "port",
    LIB / "xapi" / "minilib",
    ROOT / "include",
    ROOT / "build" / "generated",
    LIB / "xapi" / "internal" / "shims",
    ROOT / "include" / "xboxkrnl",
    ROOT / "include" / "sdk",
    ROOT / "vendor" / "picolibc" / "libc" / "include",
    ROOT / "vendor" / "picolibc" / "libc" / "machine" / "x86",
]

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+([<"])([^>"]+)[>"]', re.MULTILINE)
DEFINE_RE = re.compile(r'^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\b')
UNDEF_RE = re.compile(r'^\s*#\s*undef\s+([A-Za-z_][A-Za-z0-9_]*)\b')
IFNDEF_RE = re.compile(r'^\s*#\s*ifndef\s+([A-Za-z_][A-Za-z0-9_]*)\b')
IFDEF_RE = re.compile(r'^\s*#\s*ifdef\s+([A-Za-z_][A-Za-z0-9_]*)\b')
IF_DEFINED_RE = re.compile(r'^\s*#\s*if\s+defined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)')
IF_NOT_DEFINED_RE = re.compile(r'^\s*#\s*if\s+!\s*defined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)')
IF_EXPR_RE = re.compile(r"^\s*#\s*if\s+(.*)$")
ELSE_RE = re.compile(r"^\s*#\s*else\b")
ELIF_RE = re.compile(r"^\s*#\s*elif\b")
ENDIF_RE = re.compile(r"^\s*#\s*endif\b")
SOURCE_RE = re.compile(r'"libs/libxapi/[^"]+\.(?:c|cpp|S)"')
SLICE_SOURCES_RE = re.compile(r"pub const (\w+)_sources = \[_]\[\]const u8\{")

# profile.h force-includes for libxapi (see xapi/site/profile.h).
PROFILE_DEFINES = {
    "NOD3D",
    "NODSOUND",
    "NOWINSOCK",
}

# build.zig -D flags common to all libxapi translation units.
BUILD_DEFINES = {
    "_XAPI_",
}


def bootstrap_defines(extra: set[str] | None = None) -> set[str]:
    defines = set(PROFILE_DEFINES)
    defines.update(BUILD_DEFINES)
    profile = LIB / "xapi" / "site" / "profile.h"
    if profile.is_file():
        for line in profile.read_text(encoding="utf-8", errors="replace").splitlines():
            match = DEFINE_RE.match(line.strip())
            if match:
                defines.add(match.group(1))
    if extra:
        defines.update(extra)
    return defines


def parse_sources() -> list[Path]:
    text = SOURCES_ZIG.read_text(encoding="utf-8")
    return [ROOT / match.strip('"') for match in SOURCE_RE.findall(text)]


def resolve_include(name: str, parent: Path | None) -> Path | None:
    if parent is not None:
        candidate = (parent.parent / name).resolve()
        if candidate.is_file():
            return candidate

    for directory in INCLUDE_DIRS:
        candidate = (directory / name).resolve()
        if candidate.is_file():
            return candidate
    return None


def is_if_directive(line: str) -> bool:
    return bool(
        IFNDEF_RE.match(line)
        or IFDEF_RE.match(line)
        or IF_DEFINED_RE.match(line)
        or IF_NOT_DEFINED_RE.match(line)
        or IF_EXPR_RE.match(line)
        or ELIF_RE.match(line)
    )


def eval_defined_expr(expr: str, defines: set[str]) -> bool:
    expr = expr.strip()
    while expr.startswith("(") and expr.endswith(")"):
        expr = expr[1:-1].strip()
    if not expr:
        return False
    if "||" in expr:
        return any(eval_defined_expr(part, defines) for part in expr.split("||"))
    if "&&" in expr:
        return all(eval_defined_expr(part, defines) for part in expr.split("&&"))
    expr = expr.strip()
    if expr.startswith("!"):
        return not eval_defined_expr(expr[1:].strip(), defines)
    match = re.match(r"defined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)", expr)
    if match:
        return match.group(1) in defines
    if expr == "0":
        return False
    if expr == "1":
        return True
    return False


def directive_condition(line: str, defines: set[str]) -> bool:
    if IFNDEF_RE.match(line):
        return IFNDEF_RE.match(line).group(1) not in defines
    if IFDEF_RE.match(line):
        return IFDEF_RE.match(line).group(1) in defines
    match = IF_DEFINED_RE.match(line)
    if match:
        return match.group(1) in defines
    match = IF_NOT_DEFINED_RE.match(line)
    if match:
        return match.group(1) not in defines
    match = IF_EXPR_RE.match(line)
    if match:
        return eval_defined_expr(match.group(1), defines)
    match = ELIF_RE.match(line)
    if match:
        return eval_defined_expr(line[match.end() :].strip(), defines)
    return False


class PreprocBlock:
    __slots__ = ("parent_active", "branch_taken", "active")

    def __init__(self, parent_active: bool, branch_taken: bool, active: bool) -> None:
        self.parent_active = parent_active
        self.branch_taken = branch_taken
        self.active = active


def scan_file(path: Path, defines: set[str], reachable: set[Path], queue: deque[Path]) -> None:
    if path in reachable or not path.is_file():
        return
    reachable.add(path)
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return

    file_defines = set(defines)
    blocks: list[PreprocBlock] = []

    def current_active() -> bool:
        return blocks[-1].active if blocks else True

    for line in text.splitlines():
        stripped = line.strip()
        if ENDIF_RE.match(stripped):
            if blocks:
                blocks.pop()
            continue
        if ELIF_RE.match(stripped):
            if not blocks:
                continue
            block = blocks[-1]
            if block.branch_taken:
                block.active = False
            else:
                cond = block.parent_active and directive_condition(stripped, file_defines)
                block.branch_taken = cond
                block.active = cond
            continue
        if ELSE_RE.match(stripped):
            if not blocks:
                continue
            block = blocks[-1]
            block.active = block.parent_active and not block.branch_taken
            block.branch_taken = True
            continue
        if is_if_directive(stripped):
            parent_active = current_active()
            cond = parent_active and directive_condition(stripped, file_defines)
            blocks.append(PreprocBlock(parent_active, cond, cond))
            continue
        if not current_active():
            continue
        if DEFINE_RE.match(stripped):
            file_defines.add(DEFINE_RE.match(stripped).group(1))
            continue
        if UNDEF_RE.match(stripped):
            file_defines.discard(UNDEF_RE.match(stripped).group(1))
            continue
        match = INCLUDE_RE.match(line)
        if not match:
            continue
        include = match.group(2)
        resolved = resolve_include(
            include,
            path if include.startswith((".", "..")) or "/" in include or "\\" in include else None,
        )
        if resolved is not None and resolved not in reachable:
            queue.append(resolved)


def parse_all_slices() -> dict[str, list[Path]]:
    text = SOURCES_ZIG.read_text(encoding="utf-8")
    slices: dict[str, list[Path]] = {}
    current: str | None = None
    for line in text.splitlines():
        sources_match = SLICE_SOURCES_RE.match(line.strip())
        if sources_match:
            current = sources_match.group(1)
            slices[current] = []
            continue
        if current is not None:
            if line.strip() == "};":
                current = None
                continue
            for match in SOURCE_RE.findall(line):
                slices[current].append(ROOT / match.strip('"'))
    return slices


# Per-slice -include bridges and -D flags from libs/libxapi/build.zig.
SLICE_PROFILES: dict[str, tuple[list[Path], set[str]]] = {
    "k32": ([LIB / "xapi" / "site" / "bridge_k32.h"], set()),
    "dll": ([LIB / "xapi" / "site" / "bridge_k32.h"], set()),
    "rtl": (
        [LIB / "xapi" / "site" / "bridge_rtl.h"],
        {"_NTSYSTEM_"},
    ),
    "uuid": (
        [LIB / "xapi" / "site" / "bridge_uuid.h"],
        {"XAPI_UUID_BUILD"},
    ),
    "ohcd": (
        [LIB / "xapi" / "site" / "bridge_usb.h"],
        {
            "USE_DMA_MACROS",
            "OHCD_XBOX_HARDWARE_ONLY",
            "OHCD_ISOCHRONOUS_SUPPORTED",
        },
    ),
    "usbd": (
        [LIB / "xapi" / "site" / "bridge_usb.h"],
        set(),
    ),
    "usbhub": (
        [LIB / "xapi" / "site" / "bridge_usb.h"],
        set(),
    ),
    "mu": (
        [LIB / "xapi" / "site" / "bridge_usb.h"],
        set(),
    ),
    "xid": (
        [LIB / "xapi" / "site" / "bridge_usb.h"],
        set(),
    ),
}


def force_include_seeds(extra: list[Path] | None = None) -> list[Path]:
    """Headers always force-included by libxapi build flags (-include)."""
    seeds = [
        LIB / "xapi" / "site" / "profile.h",
        ROOT / "build" / "generated" / "picolibc.h",
    ]
    if extra:
        seeds.extend(extra)
    return seeds


def compute_reachable(
    sources: list[Path],
    extra_defines: set[str] | None = None,
    extra_seeds: list[Path] | None = None,
) -> set[Path]:
    defines = bootstrap_defines(extra_defines)
    reachable: set[Path] = set()
    queue: deque[Path] = deque(sources)
    for seed in force_include_seeds(extra_seeds):
        queue.append(seed)
    while queue:
        scan_file(queue.popleft(), defines, reachable, queue)
    return reachable


def compute_all_reachable() -> set[Path]:
    reachable: set[Path] = set()
    for slice_name, sources in parse_all_slices().items():
        extra_seeds, extra_defines = SLICE_PROFILES.get(slice_name, ([], set()))
        reachable |= compute_reachable(sources, extra_defines, extra_seeds)
    return reachable


def main() -> int:
    reachable = compute_all_reachable()

    nt_dir = (LIB / "xapi" / "nt").resolve()
    win32_dir = (LIB / "xapi" / "win32").resolve()
    all_nt = {path.resolve() for path in nt_dir.rglob("*.h")}
    used_nt = {path for path in reachable if path.is_relative_to(nt_dir)}
    unused_nt = sorted(all_nt - used_nt, key=lambda p: str(p).lower())
    all_win32 = {path.resolve() for path in win32_dir.rglob("*.h")}
    used_win32 = {path for path in reachable if path.is_relative_to(win32_dir)}
    unused_win32 = sorted(all_win32 - used_win32, key=lambda p: str(p).lower())

    print(f"Reachable headers: {len(reachable)}")
    print(f"win32/: {len(used_win32)} used / {len(all_win32)} total")
    print(f"nt/:    {len(used_nt)} used / {len(all_nt)} total")
    if unused_win32:
        print("\nUnused win32/ headers:")
        for path in unused_win32:
            print(f"  {path.relative_to(win32_dir)}")
    if unused_nt:
        print("\nUnused nt/ headers:")
        for path in unused_nt:
            print(f"  {path.relative_to(nt_dir)}")
    else:
        print("\nAll nt/ headers are reachable.")

    if "--delete" in sys.argv:
        for path in unused_nt + unused_win32:
            if path.is_file():
                path.unlink()
                print(f"deleted {path.relative_to(ROOT)}")

    if "--check" in sys.argv or "--delete" in sys.argv:
        print("\nCompile check:")
        return subprocess.run(["zig", "build", "xapi-slices"], cwd=ROOT, check=False).returncode
    return 0


if __name__ == "__main__":
    sys.exit(main())
