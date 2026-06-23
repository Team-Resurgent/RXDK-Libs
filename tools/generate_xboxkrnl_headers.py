#!/usr/bin/env python3
"""Generate cleaned xboxkrnl headers under include/xboxkrnl/.

Types and structs are extracted from the xbox_leak_may_2020 submodule
(public XDK / SDK headers). Export ordinals come from the leak
xboxkrnl.def (same path as prebuilt import lib generation).

Shipped headers contain no license banners or upstream attribution.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "include" / "xboxkrnl"
MANIFEST = ROOT / "tools" / "xboxkrnl_manifest"
LEAK = ROOT / "vendor" / "xbox_leak_may_2020" / "xbox_leak_may_2020" / "xbox trunk" / "xbox"
LEAK_DEF = (
    LEAK
    / "private"
    / "ntos"
    / "init"
    / "console"
    / "obj"
    / "i386"
    / "xboxkrnl.def"
)
LEAK_WINNT = LEAK / "public" / "xdk" / "inc" / "winnt.h"
LEAK_WINDEF = LEAK / "public" / "xdk" / "inc" / "windef.h"
LEAK_BASETSD = LEAK / "public" / "xdk" / "inc" / "basetsd.h"
LEAK_NTDEF = LEAK / "public" / "sdk" / "inc" / "ntdef.h"
LEAK_NTSTATUS = LEAK / "public" / "sdk" / "inc" / "ntstatus.h"

README = Path(ROOT / "include" / "xboxkrnl" / "README.md").read_text(encoding="utf-8")

API_GROUPS: list[tuple[str, tuple[str, ...]]] = [
    ("av", ("Av",)),
    ("dbg", ("Dbg",)),
    ("ex", ("Ex",)),
    ("hal", ("Hal", "WRITE_PORT_")),
    ("io", ("Io", "Fsc", "Idex")),
    ("ke", ("Ke",)),
    ("mm", ("Mm",)),
    ("nt", ("Nt",)),
    ("ob", ("Ob",)),
    ("ps", ("Ps",)),
    ("rtl", ("Rtl",)),
    ("xc", ("Xc", "Xe")),
    ("xbox", ("Xbox", "LaunchData")),
]

TYPE_SECTIONS: list[str] = ["common", "file", "kernel", "io", "misc"]
TYPE_SECTION_RE = re.compile(r"//\s*RXDK_TYPES:\s*(\w+)")


def read_leak(path: Path) -> str:
    if not path.is_file():
        raise FileNotFoundError(path)
    return path.read_text(encoding="utf-8", errors="replace")


def strip_c_comments(text: str) -> str:
    text = re.sub(r"/\*\+[\s\S]*?--\*/", "", text)
    text = re.sub(r"/\*\*[\s\S]*?\*/", "", text)
    text = re.sub(r"/\*[^*]*\*+(?:[^/*][^*]*\*+)*/", "", text)
    out: list[str] = []
    for line in text.splitlines():
        stripped = line.strip()
        if TYPE_SECTION_RE.match(stripped):
            out.append(stripped)
            continue
        if "//" in line:
            line = line.split("//", 1)[0].rstrip()
        out.append(line.rstrip())
    return "\n".join(out).strip()


def find_matching_brace(text: str, open_index: int) -> int:
    depth = 0
    for i in range(open_index, len(text)):
        ch = text[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return i
    raise ValueError("unbalanced braces")


def extract_statement(source: str, start: int) -> tuple[str, int]:
    semi = source.find(";", start)
    if semi < 0:
        raise ValueError("missing semicolon")
    return source[start : semi + 1], semi + 1


def extract_typedef_struct(source: str, tag: str) -> str | None:
    m = re.search(rf"typedef\s+struct\s+_{re.escape(tag)}\b", source)
    if not m:
        return None
    brace = source.find("{", m.end())
    if brace < 0:
        return None
    end_brace = find_matching_brace(source, brace)
    semi = source.find(";", end_brace)
    if semi < 0:
        return None
    return source[m.start() : semi + 1]


def extract_define(source: str, name: str) -> str | None:
    m = re.search(rf"^#define\s+{re.escape(name)}\b[^\n]*", source, re.MULTILINE)
    return m.group(0).strip() if m else None


def extract_typedef_alias(source: str, name: str) -> str | None:
    m = re.search(rf"typedef\s+[^;]+\b{re.escape(name)}\s*,[^;]*;", source)
    if m:
        return m.group(0).strip()
    m = re.search(rf"typedef\s+[^;]+\b{re.escape(name)}\s*;", source)
    return m.group(0).strip() if m else None


def extract_from_leak(sources: list[str], *, struct: str | None = None, define: str | None = None, typedef: str | None = None) -> str | None:
    for src in sources:
        if struct:
            got = extract_typedef_struct(src, struct)
            if got:
                return got
        if define:
            got = extract_define(src, define)
            if got:
                return got
        if typedef:
            got = extract_typedef_alias(src, typedef)
            if got:
                return got
    return None


def leak_sources() -> dict[str, str]:
    return {
        "winnt": read_leak(LEAK_WINNT),
        "windef": read_leak(LEAK_WINDEF),
        "basetsd": read_leak(LEAK_BASETSD),
        "ntdef": read_leak(LEAK_NTDEF),
        "ntstatus": read_leak(LEAK_NTSTATUS),
    }


def manifest_text(name: str) -> str:
    path = MANIFEST / name
    if not path.is_file():
        raise FileNotFoundError(path)
    return path.read_text(encoding="utf-8")


def manifest_struct_tags(manifest: str) -> list[str]:
    return re.findall(r"typedef struct _(\w+)", manifest)


def manifest_define_names(manifest: str) -> list[str]:
    names: list[str] = []
    for line in manifest.splitlines():
        m = re.match(r"#define\s+(\w+)", line.strip())
        if m:
            names.append(m.group(1))
    return names


def manifest_status_names(manifest: str) -> list[str]:
    return re.findall(r"#define\s+(STATUS_\w+)", manifest)


def write_header(name: str, body: str) -> None:
    path = OUT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(body.rstrip() + "\n", encoding="utf-8", newline="\n")
    print(f"wrote {path.relative_to(ROOT)}")


def add_packed_if_context(text: str, tag: str) -> str:
    if tag not in ("FLOATING_SAVE_AREA", "CONTEXT"):
        return text
    text = text.replace("SIZE_OF_FX_REGISTERS", "128")
    if tag == "CONTEXT" and "*PCONTEXT" not in text:
        text = re.sub(
            r"(\}\s*(?:__attribute__\(\(packed\)\)\s*)?)CONTEXT\s*;",
            r"\1CONTEXT, *PCONTEXT;",
            text,
            count=1,
        )
    if "__attribute__" in text or "__declspec" in text:
        return text
    return re.sub(r"\}\s*(\w)", r"} __attribute__((packed)) \1", text, count=1)


def sanitize_leak(text: str) -> str:
    text = re.sub(r"#ifdef MIDL_PASS[\s\S]*?#endif\s*", "", text)
    text = re.sub(r"\[(?:public|size_is|length_is)[^\]]*\]\s*", "", text, flags=re.IGNORECASE)
    text = re.sub(r", \*RESTRICTED_POINTER PRLIST_ENTRY", "", text)
    text = re.sub(r"\*RESTRICTED_POINTER\s+", "*", text)
    return strip_c_comments(text)


def strip_manifest_guards(body: str, guard: tuple[str, str]) -> str:
    if guard[0] in body:
        body = re.sub(rf"^#ifndef\s+{re.escape(guard[0])}\s*\n", "", body)
        body = re.sub(rf"^#define\s+{re.escape(guard[1])}\s*\n", "", body)
        body = re.sub(r"#endif\s*$", "", body.rstrip()).rstrip()
    return body


def strip_includes(body: str, *headers: str) -> str:
    for header in headers:
        body = re.sub(rf'#include\s+[<"]{re.escape(header)}[>"]\s*\n', "", body)
    return body.strip()


def dedupe_includes(body: str) -> str:
    seen: set[str] = set()
    out: list[str] = []
    for line in body.splitlines():
        if line.startswith("#include"):
            if line in seen:
                continue
            seen.add(line)
        out.append(line)
    return "\n".join(out).strip()


def normalize_body(body: str) -> str:
    body = dedupe_includes(body)
    lines = body.splitlines()
    out: list[str] = []
    prev_blank = False
    for line in lines:
        stripped = line.rstrip()
        if not stripped:
            if prev_blank:
                continue
            out.append("")
            prev_blank = True
            continue
        out.append(stripped)
        prev_blank = False
    return "\n".join(out).strip()


def replace_structs_from_leak(
    body: str,
    src: dict[str, str],
    *,
    allow: set[str] | None = None,
) -> str:
    for tag in re.findall(r"typedef struct _(\w+)", body):
        if allow is not None and tag not in allow:
            continue
        got = extract_from_leak([src["ntdef"], src["winnt"]], struct=tag)
        if not got:
            continue
        got = add_packed_if_context(sanitize_leak(got), tag)
        old = extract_typedef_struct(body, tag)
        if old:
            body = body.replace(old, got)
    return body


def overlay_manifest(
    manifest_name: str,
    src: dict[str, str],
    *,
    guard: tuple[str, str],
    struct_allow: set[str] | None = None,
    define_skip_prefixes: tuple[str, ...] = (),
) -> str:
    manifest = strip_c_comments(strip_manifest_preamble(manifest_text(manifest_name)))
    body = replace_structs_from_leak(manifest, src, allow=struct_allow)
    body = body.replace("SIZE_OF_FX_REGISTERS", "128")
    for name in manifest_define_names(body):
        if any(name.startswith(prefix) for prefix in define_skip_prefixes):
            continue
        got = extract_from_leak([src["winnt"], src["ntdef"], src["windef"]], define=name)
        if got:
            old = extract_define(body, name)
            if old:
                body = body.replace(old, sanitize_leak(got))
    body = strip_manifest_guards(body, guard)
    body = format_header_body(body)
    return f"#ifndef {guard[0]}\n#define {guard[1]}\n\n{body}\n\n#endif\n"


def generate_xboxdef(src: dict[str, str]) -> str:
    return overlay_manifest(
        "xboxdef.h",
        src,
        guard=("__XBOXDEF_H__", "__XBOXDEF_H__"),
        struct_allow={"FLOATING_SAVE_AREA", "CONTEXT", "LARGE_INTEGER", "ULARGE_INTEGER"},
        define_skip_prefixes=("CONTEXT_", "EXCEPTION_", "FILE_ATTRIBUTE_", "INVALID_FILE_ATTRIBUTES"),
    )


def generate_winnt_pe(src: dict[str, str]) -> str:
    manifest = strip_c_comments(strip_manifest_preamble(manifest_text("winnt_pe.h")))
    body = replace_structs_from_leak(
        manifest,
        src,
        allow={
            "IMAGE_DOS_HEADER",
            "IMAGE_DATA_DIRECTORY",
            "IMAGE_OPTIONAL_HEADER",
            "IMAGE_FILE_HEADER",
            "IMAGE_NT_HEADERS32",
            "IMAGE_SECTION_HEADER",
            "IMAGE_IMPORT_DESCRIPTOR",
            "IMAGE_EXPORT_DIRECTORY",
            "IMAGE_TLS_DIRECTORY32",
            "EXCEPTION_POINTERS",
        },
    )
    for name in manifest_define_names(body):
        got = extract_from_leak([src["winnt"], src["ntdef"]], define=name)
        if got:
            old = extract_define(body, name)
            if old:
                body = body.replace(old, sanitize_leak(got))
    body = format_header_body(body)
    return (
        "#ifndef __XBOXKRNL_WINNT_PE_H__\n"
        "#define __XBOXKRNL_WINNT_PE_H__\n\n"
        "#include <xboxkrnl/xboxdef.h>\n\n"
        f"{body}\n\n"
        "#endif\n"
    )


def generate_winnt_xbe(src: dict[str, str]) -> str:
    manifest = strip_c_comments(strip_manifest_preamble(manifest_text("winnt_xbe.h")))
    body = format_header_body(manifest)
    return (
        "#ifndef __XBOXKRNL_WINNT_XBE_H__\n"
        "#define __XBOXKRNL_WINNT_XBE_H__\n\n"
        "#include <xboxkrnl/winnt/pe.h>\n\n"
        f"{body}\n\n"
        "#endif\n"
    )


def generate_winnt_umbrella() -> str:
    return (
        "#ifndef __WINNT_H__\n"
        "#define __WINNT_H__\n\n"
        "#include <xboxkrnl/winnt/pe.h>\n"
        "#include <xboxkrnl/winnt/xbe.h>\n\n"
        "#endif\n"
    )


def parse_status_define(line: str) -> tuple[str, str] | None:
    match = re.match(r"#define\s+(STATUS_\w+)\s+(.*)", line.strip())
    if not match:
        return None
    return match.group(1), match.group(2).strip()


def parse_define_line(line: str) -> tuple[str, str] | None:
    match = re.match(r"#define\s+(\S+)\s+(.*)", line.lstrip())
    if not match:
        return None
    return match.group(1), match.group(2).rstrip()


def align_consecutive_defines(lines: list[str]) -> list[str]:
    out: list[str] = []
    index = 0
    while index < len(lines):
        line = lines[index]
        stripped = line.lstrip()
        if not stripped.startswith("#define"):
            out.append(line)
            index += 1
            continue
        group: list[tuple[str, str]] = []
        while index < len(lines):
            parsed = parse_define_line(lines[index])
            if parsed is None:
                if not group:
                    out.append(lines[index])
                    index += 1
                break
            group.append(parsed)
            index += 1
        if len(group) >= 2:
            width = max(len(name) for name, _ in group)
            for name, value in group:
                out.append(f"#define {name.ljust(width)} {value}")
        elif group:
            name, value = group[0]
            out.append(f"#define {name} {value}")
    return out


def format_status_defines(entries: list[tuple[str, str]]) -> list[str]:
    if not entries:
        return []
    width = max(len(name) for name, _ in entries)
    return [f"#define {name.ljust(width)} {value}" for name, value in entries]


def generate_ntstatus(src: dict[str, str]) -> str:
    manifest = manifest_text("ntstatus.h")
    entries: list[tuple[str, str]] = []
    for name in manifest_status_names(manifest):
        got = extract_define(src["ntstatus"], name)
        if got:
            got = re.sub(r"\(\(NTSTATUS\)", "((DWORD)", got)
            got = strip_c_comments(got)
            parsed = parse_status_define(got)
            if parsed:
                entries.append(parsed)
                continue
        match = re.search(rf"#define\s+({re.escape(name)})\s+(.*)", manifest)
        if match:
            entries.append((match.group(1), match.group(2).strip()))
    lines = [
        "#ifndef _NTSTATUS_",
        "#define _NTSTATUS_",
        "",
        "#include <xboxkrnl/xboxdef.h>",
        "",
    ]
    lines.extend(format_status_defines(entries))
    lines.extend(["", "#endif", ""])
    return "\n".join(lines)


def strip_manifest_preamble(text: str) -> str:
    text = re.sub(r"^//[^\n]*\n(?://[^\n]*\n)*", "", text)
    while True:
        m = re.match(r"/\*\*[\s\S]*?\*/\s*", text)
        if not m:
            break
        text = text[m.end() :].lstrip()
    if text.startswith("#pragma once"):
        text = text.split("\n", 1)[1].lstrip()
    return text


def strip_kernel_manifest_wrapper(manifest: str) -> str:
    manifest = strip_manifest_preamble(manifest)
    manifest = strip_c_comments(manifest)
    manifest = re.sub(r"#pragma once\s*", "", manifest)
    manifest = re.sub(r"#include\s+<xboxkrnl/xboxdef\.h>\s*", "", manifest)
    manifest = re.sub(r"#include\s+<xboxkrnl/ntstatus\.h>\s*", "", manifest)
    manifest = re.sub(r"#pragma clang diagnostic push\s*", "", manifest)
    manifest = re.sub(
        r'#pragma clang diagnostic ignored "-Wlanguage-extension-token"\s*',
        "",
        manifest,
    )
    manifest = re.sub(r"#pragma ms_struct on\s*", "", manifest)
    manifest = re.sub(r"#pragma ms_struct off\s*", "", manifest)
    manifest = re.sub(r"#pragma clang diagnostic pop\s*", "", manifest)
    manifest = re.sub(
        r"#if\s+defined\(__cplusplus\)\s*extern\s+\"C\"\s*\{\s*#endif\s*",
        "",
        manifest,
    )
    manifest = re.sub(r"#endif\s*/\*\s*__cplusplus\s*\*/\s*", "", manifest)
    manifest = re.sub(
        r"#ifdef XBOXKRNL_PREFER_KERNEL_EXPORT\s*([\s\S]*?)#else\s*[\s\S]*?#endif",
        r"\1",
        manifest,
    )
    return manifest.strip()


def line_kind(line: str) -> str:
    stripped = line.strip()
    if not stripped:
        return "blank"
    if stripped.startswith("#if"):
        return "if"
    if stripped.startswith("#define"):
        return "define"
    if stripped.startswith("typedef enum"):
        return "enum"
    if stripped.startswith("typedef struct") or stripped.startswith("typedef union"):
        return "struct"
    if stripped.startswith("typedef"):
        return "typedef"
    if stripped.startswith("struct ") and stripped.endswith(";"):
        return "forward"
    return "other"


def format_readable(text: str) -> str:
    lines = text.splitlines()
    out: list[str] = []
    prev_kind: str | None = None
    in_multiline_define = False
    in_preprocessor = False
    depth = 0
    for line in lines:
        stripped = line.rstrip()
        content = stripped.lstrip()
        depth += content.count("{") - content.count("}")
        depth = max(depth, 0)
        if content.startswith("#if") or content.startswith("#ifdef") or content.startswith("#ifndef"):
            in_preprocessor = True
        elif content.startswith("#endif"):
            in_preprocessor = False
        if content.startswith("#define"):
            in_multiline_define = content.endswith("\\") or (
                "{" in content and "}" not in content
            )
            kind = "define"
        elif in_multiline_define:
            kind = "define"
            if content.endswith("\\"):
                in_multiline_define = True
            elif "}" in content:
                in_multiline_define = False
        else:
            kind = line_kind(content)
        at_top_level = depth == 0 or (depth == 1 and content.startswith("}"))
        if (
            at_top_level
            and out
            and kind != "blank"
            and prev_kind
            and prev_kind != "blank"
            and kind != prev_kind
            and not (kind == "define" and prev_kind == "define")
            and not (kind == "typedef" and prev_kind == "typedef")
            and not (content == "{" and prev_kind == "struct")
            and not in_preprocessor
            and not in_multiline_define
        ):
            out.append("")
        out.append(stripped)
        prev_kind = kind
    return "\n".join(out)


def tighten_preprocessor_blocks(lines: list[str]) -> list[str]:
    out: list[str] = []
    depth = 0
    for line in lines:
        stripped = line.lstrip()
        if stripped.startswith("#if"):
            while out and out[-1] == "":
                out.pop()
            out.append(line)
            depth += 1
            continue
        if stripped.startswith("#else") or stripped.startswith("#elif"):
            while out and out[-1] == "":
                out.pop()
            out.append(line)
            depth = max(depth, 1)
            continue
        if stripped.startswith("#endif"):
            while out and out[-1] == "":
                out.pop()
            out.append(line)
            depth = max(depth - 1, 0)
            continue
        if depth > 0 and (stripped.startswith("#define") or stripped.startswith("typedef")):
            while out and out[-1] == "":
                out.pop()
        out.append(line)
    return out


def trim_spurious_blanks(lines: list[str]) -> list[str]:
    out: list[str] = []
    for index, line in enumerate(lines):
        stripped = line.lstrip()
        if (
            stripped == ""
            and index + 1 < len(lines)
            and line == ""
            and out
            and out[-1].rstrip().endswith("{")
        ):
            continue
        out.append(line)
    return out


def format_types_layout(text: str) -> str:
    """Strip section markers and comments; separate top-level declarations; preserve indentation."""
    lines = text.splitlines()
    out: list[str] = []
    index = 0

    def append_block(block: list[str], *, trailing_blank: bool = False) -> None:
        nonlocal out
        if out and out[-1].strip():
            out.append("")
        out.extend(block)
        if trailing_blank:
            out.append("")

    while index < len(lines):
        raw = lines[index]
        line = raw.rstrip()
        stripped = line.lstrip()
        if not stripped or TYPE_SECTION_RE.match(stripped):
            index += 1
            continue

        if stripped.startswith("#define") and "{" in stripped and "}" not in stripped:
            block = [line]
            index += 1
            while index < len(lines):
                next_line = lines[index].rstrip()
                if TYPE_SECTION_RE.match(next_line.lstrip()):
                    break
                block.append(next_line)
                if "}" in next_line:
                    index += 1
                    break
                index += 1
            if out and out[-1].strip() and not out[-1].strip().startswith("#define"):
                out.append("")
            out.extend(block)
            continue

        if re.match(r"typedef\s+(struct|union|enum)\b", stripped):
            block = [line]
            depth = stripped.count("{") - stripped.count("}")
            index += 1
            while index < len(lines) and (depth > 0 or ";" not in block[-1]):
                next_line = lines[index].rstrip()
                next_stripped = next_line.lstrip()
                if TYPE_SECTION_RE.match(next_stripped):
                    break
                if next_stripped == "{" and block[-1].lstrip().startswith("typedef"):
                    block[-1] = f"{block[-1]} {{"
                    depth += 1
                    index += 1
                    continue
                block.append(next_line)
                depth += next_stripped.count("{") - next_stripped.count("}")
                index += 1
                if ";" in next_stripped and depth <= 0:
                    break
            append_block(block, trailing_blank=True)
            continue

        if stripped.startswith("typedef") and stripped.endswith(";"):
            if out and out[-1].strip() and not out[-1].lstrip().startswith("typedef"):
                out.append("")
            out.append(line)
            index += 1
            continue

        if stripped.startswith("typedef"):
            block = [line]
            index += 1
            while index < len(lines):
                next_line = lines[index].rstrip()
                if TYPE_SECTION_RE.match(next_line.lstrip()):
                    break
                block.append(next_line)
                index += 1
                if next_line.lstrip().endswith(";"):
                    break
            append_block(block, trailing_blank=True)
            continue

        if re.match(r"struct\s+_\w+\s*;", stripped):
            append_block([line], trailing_blank=True)
            index += 1
            continue

        if stripped.startswith("#define"):
            prev = out[-1].strip() if out else ""
            if out and prev and not prev.startswith("#define") and not prev.startswith(("#ifndef", "#ifdef", "#if")):
                out.append("")
            out.append(line)
            index += 1
            continue

        if stripped.startswith("#if"):
            if out and out[-1].strip():
                out.append("")
            out.append(line)
            index += 1
            continue

        out.append(line)
        index += 1

    return "\n".join(out)


def format_header_body(text: str) -> str:
    text = strip_c_comments(text)
    text = format_types_layout(text)
    text = format_readable(text)
    lines = trim_spurious_blanks(tighten_preprocessor_blocks(text.splitlines()))
    lines = align_consecutive_defines(lines)
    return normalize_body("\n".join(lines))


def wrap_header(guard: str, body: str, *, readable: bool = True) -> str:
    body = normalize_body(format_readable(body) if readable else body)
    return f"#ifndef {guard}\n#define {guard}\n\n{body}\n\n#endif\n"


def split_type_sections(types: str) -> dict[str, str]:
    sections: dict[str, list[str]] = {name: [] for name in TYPE_SECTIONS}
    current = TYPE_SECTIONS[0]
    for line in types.splitlines():
        marker = TYPE_SECTION_RE.match(line.strip())
        if marker:
            current = marker.group(1)
            sections.setdefault(current, [])
            continue
        sections.setdefault(current, []).append(line)
    return {name: "\n".join(lines).strip() for name, lines in sections.items() if lines}


def generate_type_part(name: str, body: str) -> str:
    guard = f"XBOXKRNL_TYPES_{name.upper()}_H"
    return wrap_header(guard, format_header_body(body), readable=False)


def generate_types_umbrella(parts: list[str]) -> str:
    includes = [f"#include <xboxkrnl/types/{part}.h>" for part in parts]
    body = "\n".join(includes)
    return wrap_header("XBOXKRNL_TYPES_H", body, readable=False)


def write_types_tree(type_sections: dict[str, str]) -> list[str]:
    types_dir = OUT / "types"
    if types_dir.is_dir():
        for old in types_dir.glob("*.h"):
            old.unlink()
    else:
        types_dir.mkdir(parents=True, exist_ok=True)

    written: list[str] = []
    for name in TYPE_SECTIONS:
        if name not in type_sections or not type_sections[name].strip():
            continue
        rel = f"types/{name}.h"
        write_header(rel, generate_type_part(name, type_sections[name]))
        written.append(name)
    write_header("types.h", generate_types_umbrella(written))
    return written


def extract_export_symbol(decl: str) -> str:
    tail = decl.split("XBAPI", 1)[-1]
    match = re.search(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*(?:\(|;|\[)", tail)
    if not match:
        raise ValueError(f"could not parse export symbol from: {decl[:80]!r}")
    return match.group(1)


def split_types_and_exports(body: str) -> tuple[str, list[tuple[str, str]]]:
    lines = body.splitlines()
    split_at: int | None = None
    for index, line in enumerate(lines):
        if line.startswith("XBAPI"):
            split_at = index
            break
    if split_at is None:
        return body, []

    types = "\n".join(lines[:split_at]).strip()
    api_lines = lines[split_at:]
    exports: list[tuple[str, str]] = []
    index = 0
    while index < len(api_lines):
        line = api_lines[index].strip()
        if not line.startswith("XBAPI"):
            index += 1
            continue
        start = index
        if line.endswith(";") and "(" not in line:
            index += 1
        else:
            while index < len(api_lines) and not api_lines[index].strip().endswith(");"):
                index += 1
            index += 1
        decl = normalize_body("\n".join(api_lines[start:index]))
        exports.append((extract_export_symbol(decl), decl))
    return types, exports


def api_group_for(name: str) -> str:
    for group, prefixes in API_GROUPS:
        for prefix in prefixes:
            if name.startswith(prefix):
                return group
    return "misc"


def generate_api_headers(exports: list[tuple[str, str]]) -> dict[str, str]:
    grouped: dict[str, list[tuple[str, str]]] = {name: [] for name, _ in API_GROUPS}
    grouped["misc"] = []
    for symbol, decl in exports:
        grouped[api_group_for(symbol)].append((symbol, decl))

    headers: dict[str, str] = {}
    for group, _prefixes in API_GROUPS:
        items = sorted(grouped[group], key=lambda item: item[0].lower())
        if not items:
            continue
        body = "\n\n".join(decl for _, decl in items)
        guard = f"XBOXKRNL_API_{group.upper()}_H"
        headers[f"api/{group}.h"] = wrap_header(guard, body)

    misc = sorted(grouped["misc"], key=lambda item: item[0].lower())
    if misc:
        body = "\n\n".join(decl for _, decl in misc)
        headers["api/misc.h"] = wrap_header("XBOXKRNL_API_MISC_H", body)
    return headers


def generate_xboxkrnl_umbrella(api_files: list[str]) -> str:
    includes = [
        "#include <xboxkrnl/xboxdef.h>",
        "#include <xboxkrnl/ntstatus.h>",
        "",
        "#ifndef ANYSIZE_ARRAY",
        "#define ANYSIZE_ARRAY 1",
        "#endif",
        "",
        "#pragma clang diagnostic push",
        '#pragma clang diagnostic ignored "-Wlanguage-extension-token"',
        "",
        "#pragma ms_struct on",
        "",
        "#if defined(__cplusplus)",
        "extern \"C\" {",
        "#endif",
        "",
        "#include <xboxkrnl/types.h>",
    ]
    for path in api_files:
        includes.append(f"#include <xboxkrnl/{path}>")
    includes.extend(
        [
            "",
            "#if defined(__cplusplus)",
            "}",
            "#endif",
            "",
            "#pragma ms_struct off",
            "#pragma clang diagnostic pop",
            "",
        ]
    )
    body = "\n".join(includes)
    return wrap_header("_XBOXKRNL_H_", body, readable=False)


def write_api_tree(api_headers: dict[str, str]) -> list[str]:
    api_dir = OUT / "api"
    if api_dir.is_dir():
        for old in api_dir.glob("*.h"):
            old.unlink()
    else:
        api_dir.mkdir(parents=True, exist_ok=True)

    written: list[str] = []
    order = [group for group, _ in API_GROUPS]
    if "misc" not in order:
        order.append("misc")
    for group in order:
        rel = f"api/{group}.h"
        if rel in api_headers:
            write_header(rel, api_headers[rel])
            written.append(rel)
    return written


def generate_xboxkrnl_split() -> tuple[str, list[str]]:
    manifest = strip_kernel_manifest_wrapper(manifest_text("xboxkrnl.h"))
    types, exports = split_types_and_exports(manifest)
    write_types_tree(split_type_sections(types))
    api_headers = generate_api_headers(exports)
    api_files = write_api_tree(api_headers)
    umbrella = generate_xboxkrnl_umbrella(api_files)
    return umbrella, api_files


def parse_leak_def(path: Path) -> list[tuple[int, str, bool]]:
    if not path.is_file():
        print(f"warning: leak def not found: {path}", file=sys.stderr)
        return []
    exports: list[tuple[int, str, bool]] = []
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.split(";")[0].strip()
        if not line or line.startswith(("NAME", "EXPORTS", "LIBRARY")):
            continue
        m = re.match(
            r"(@?[\w]+)\s+@(\d+)\s+NONAME(?:\s+CONSTANT)?",
            line,
            re.IGNORECASE,
        )
        if not m:
            continue
        name = m.group(1).lstrip("@")
        ordinal = int(m.group(2))
        is_data = "CONSTANT" in raw.upper()
        exports.append((ordinal, name, is_data))
    return exports


def generate_ordinals(exports: list[tuple[int, str, bool]]) -> str:
    lines = [
        "#ifndef RXDK_XBOXKRNL_ORDINALS_H",
        "#define RXDK_XBOXKRNL_ORDINALS_H",
        "",
    ]
    for ordinal, name, is_data in sorted(exports, key=lambda t: t[0]):
        suffix = " /* DATA */" if is_data else ""
        lines.append(f"#define RXDK_ORDINAL_{name} {ordinal}{suffix}")
    lines.extend(["", "#endif", ""])
    return "\n".join(lines)


def main() -> int:
    required = [
        LEAK_DEF,
        LEAK_WINNT,
        LEAK_WINDEF,
        LEAK_NTDEF,
        LEAK_NTSTATUS,
        MANIFEST / "xboxdef.h",
        MANIFEST / "winnt_pe.h",
        MANIFEST / "winnt_xbe.h",
        MANIFEST / "ntstatus.h",
        MANIFEST / "xboxkrnl.h",
    ]
    missing = [p for p in required if not p.is_file()]
    if missing:
        for p in missing:
            print(f"missing: {p}", file=sys.stderr)
        print("Run: git submodule update --init vendor/xbox_leak_may_2020", file=sys.stderr)
        return 1

    src = leak_sources()
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "README.md").write_text(README, encoding="utf-8", newline="\n")

    write_header("xboxdef.h", generate_xboxdef(src))
    write_header("winnt/pe.h", generate_winnt_pe(src))
    write_header("winnt/xbe.h", generate_winnt_xbe(src))
    write_header("winnt.h", generate_winnt_umbrella())
    write_header("ntstatus.h", generate_ntstatus(src))
    umbrella, _api_files = generate_xboxkrnl_split()
    write_header("xboxkrnl.h", umbrella)

    exports = parse_leak_def(LEAK_DEF)
    write_header("xboxkrnl_ordinals.h", generate_ordinals(exports))
    print(f"leak def exports: {len(exports)}")
    verify_def_coverage(exports)
    return 0


def verify_def_coverage(exports: list[tuple[int, str, bool]]) -> None:
    parts = [OUT / "types.h", OUT / "xboxkrnl.h"]
    types_dir = OUT / "types"
    if types_dir.is_dir():
        parts.extend(sorted(types_dir.glob("*.h")))
    parts.extend(sorted((OUT / "api").glob("*.h")))
    combined = "\n".join(part.read_text(encoding="utf-8") for part in parts if part.is_file())
    missing: list[str] = []
    for _, name, is_data in exports:
        if is_data:
            if not re.search(rf"\b{re.escape(name)}\b", combined):
                missing.append(name)
        elif not re.search(rf"\b{re.escape(name)}\s*\(", combined):
            missing.append(name)
    if missing:
        print(f"warning: {len(missing)} def exports missing from headers", file=sys.stderr)
        print(f"  sample: {missing[:10]}", file=sys.stderr)
    else:
        print(f"def coverage: all {len(exports)} exports declared in xboxkrnl headers")


if __name__ == "__main__":
    raise SystemExit(main())
