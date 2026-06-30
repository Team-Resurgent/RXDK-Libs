#!/usr/bin/env python3
"""Generate a decorated import .def for libkernel from authoritative leak artifacts.

Inputs (authoritative):
  - DEF: leak console xboxkrnl.def  -> name, ordinal, CONSTANT(data) flag, `=alias`
  - MAP: D:\\Git\\kernelmap\\xboxkrnl.map -> decorated symbol (_Name@N / @Name@N / _Name)

The DEF is the export whitelist (367 entries); the MAP supplies decoration + @N +
calling convention. We join on the stripped base name. MSVC /MAP has no ordinal
section, so ordinals come only from the DEF.

Default mode is a read-only dry run: prints a summary, every coverage gap, and all
alias/edge cases, and writes the full proposed table to the scratchpad. Pass
--emit-def PATH to also write the decorated .def.
"""
import argparse
import re
import sys
from pathlib import Path

DEF_DEFAULT = Path(
    r"vendor/xbox_leak_may_2020/xbox_leak_may_2020/xbox trunk/xbox/private/"
    r"ntos/init/console/obj/i386/xboxkrnl.def"
)
MAP_DEFAULT = Path(r"D:/Git/kernelmap/xboxkrnl.map")


class DefEntry:
    __slots__ = ("name", "ordinal", "constant", "alias")

    def __init__(self, name, ordinal, constant, alias):
        self.name = name
        self.ordinal = ordinal
        self.constant = constant
        self.alias = alias  # target C name (RHS of `=`) or None


def parse_def(path: Path):
    entries = []
    in_exports = False
    for raw in path.read_text(errors="replace").splitlines():
        line = raw.strip()
        if not line:
            continue
        if line.upper().startswith("EXPORTS"):
            in_exports = True
            continue
        if not in_exports or line.upper().startswith("NAME "):
            continue
        # forms:
        #   Name @ord NONAME [CONSTANT]
        #   Name = Target @ord NONAME
        m = re.match(
            r"^(?P<name>\S+)\s*(?:=\s*(?P<alias>\S+)\s+)?@(?P<ord>\d+)\s+NONAME"
            r"(?P<const>\s+CONSTANT)?\s*$",
            line,
        )
        if not m:
            print(f"  [def] UNPARSED: {line!r}", file=sys.stderr)
            continue
        entries.append(
            DefEntry(
                m.group("name"),
                int(m.group("ord")),
                bool(m.group("const")),
                m.group("alias"),
            )
        )
    return entries


# map symbol line: " SECT:OFFSET   SYMBOL   RVA   [f]   Lib:Object"
MAP_LINE = re.compile(
    r"^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(?P<sym>\S+)\s+[0-9A-Fa-f]{8}\s+"
    r"(?P<f>f\s+)?(?P<obj>\S+)\s*$"
)


def decode_symbol(sym: str, is_func: bool):
    """Return (base_name, conv, argbytes) or None for symbols we don't model."""
    if sym.startswith("?"):
        return None  # C++ mangled
    if sym.startswith("@"):  # fastcall: @Name@N
        rest = sym[1:]
        if "@" not in rest:
            return None
        base, n = rest.rsplit("@", 1)
        if not n.isdigit():
            return None
        return base, "fastcall", int(n)
    if sym.startswith("_"):
        rest = sym[1:]
        if "@" in rest:  # stdcall: _Name@N
            base, n = rest.rsplit("@", 1)
            if n.isdigit():
                return base, "stdcall", int(n)
            return None
        # no @N: cdecl function (has f) or data export (no f)
        return rest, ("cdecl" if is_func else "data"), None
    return None


def parse_map(path: Path):
    """base_name -> list of (symbol, conv, argbytes). Also exact symbol set."""
    by_base = {}
    exact = {}
    started = False
    for raw in path.read_text(errors="replace").splitlines():
        if not started:
            if "Publics by Value" in raw:
                started = True
            continue
        m = MAP_LINE.match(raw)
        if not m:
            continue
        sym = m.group("sym")
        is_func = m.group("f") is not None
        exact[sym] = is_func
        dec = decode_symbol(sym, is_func)
        if dec is None:
            continue
        base, conv, n = dec
        by_base.setdefault(base, []).append((sym, conv, n))
    return by_base, exact


# Exports that are genuinely __cdecl (varargs) -> map symbol is _Name with no @N
# and the `f` flag, so it decodes as "cdecl". Not a warning.
KNOWN_CDECL = {"DbgPrint"}


def resolve(entry: DefEntry, by_base, exact):
    """Return dict(decorated, conv, argbytes, note) or note-only on failure."""
    if entry.alias:  # Name = _snprintf / Name = sprintf  -> decorated is '_'+target
        want = "_" + entry.alias
        if want in exact:
            return dict(decorated=want, conv="cdecl-alias", argbytes=None, note="alias")
        return dict(decorated=None, conv=None, argbytes=None,
                    note=f"ALIAS-MISS target={want}")

    cands = by_base.get(entry.name, [])
    if not cands:
        return dict(decorated=None, conv=None, argbytes=None, note="GAP-no-map-symbol")

    if entry.constant:
        for sym, conv, n in cands:
            if conv == "data":
                return dict(decorated=sym, conv=conv, argbytes=n, note="data")
        # CONSTANT in def but only func symbols found
        sym, conv, n = cands[0]
        return dict(decorated=sym, conv=conv, argbytes=n,
                    note=f"WARN const-but-{conv}")

    # function: prefer stdcall/fastcall (has @N); else cdecl
    for want in ("stdcall", "fastcall"):
        for sym, conv, n in cands:
            if conv == want:
                return dict(decorated=sym, conv=conv, argbytes=n, note="")
    sym, conv, n = cands[0]
    if entry.name in KNOWN_CDECL and conv == "cdecl":
        return dict(decorated=sym, conv=conv, argbytes=n, note="cdecl-varargs")
    return dict(decorated=sym, conv=conv, argbytes=n,
                note=f"WARN func-resolved-as-{conv}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--def", dest="defp", type=Path, default=DEF_DEFAULT)
    ap.add_argument("--map", dest="mapp", type=Path, default=MAP_DEFAULT)
    ap.add_argument("--emit-def", type=Path, default=None)
    ap.add_argument("--table", type=Path, default=None,
                    help="write full proposed table here")
    args = ap.parse_args()

    if not args.defp.exists():
        sys.exit(f"def not found: {args.defp}")
    if not args.mapp.exists():
        sys.exit(f"map not found: {args.mapp}")

    defs = parse_def(args.defp)
    by_base, exact = parse_map(args.mapp)

    rows, gaps, edges = [], [], []
    counts = {"stdcall": 0, "fastcall": 0, "cdecl-alias": 0, "data": 0, "other": 0}
    for e in sorted(defs, key=lambda x: x.ordinal):
        r = resolve(e, by_base, exact)
        rows.append((e, r))
        if r["decorated"] is None:
            gaps.append((e, r))
        elif r["note"] not in ("", "data", "alias", "cdecl-varargs"):
            edges.append((e, r))
        counts[r["conv"] if r["conv"] in counts else "other"] += 1

    # full table
    lines = [f"{'ord':>4}  {'name':<34} {'decorated':<34} {'conv':<12} note"]
    for e, r in rows:
        lines.append(
            f"{e.ordinal:>4}  {e.name:<34} {str(r['decorated']):<34} "
            f"{str(r['conv']):<12} {r['note']}"
        )
    table_text = "\n".join(lines)
    table_path = args.table or Path(
        r"C:/Users/eq2k/AppData/Local/Temp/claude/D--Git-RXDK-LibsZig/"
        r"e4ae3696-05c0-4f25-88c0-48d64b98ea48/scratchpad/libkernel_table.txt"
    )
    try:
        table_path.parent.mkdir(parents=True, exist_ok=True)
        table_path.write_text(table_text + "\n")
    except OSError:
        table_path = None

    print(f"DEF exports parsed : {len(defs)}")
    print(f"MAP base symbols   : {len(by_base)}")
    print(f"resolved           : {len(defs) - len(gaps)} / {len(defs)}")
    print(f"  stdcall  : {counts['stdcall']}")
    print(f"  fastcall : {counts['fastcall']}")
    print(f"  data     : {counts['data']}")
    print(f"  alias    : {counts['cdecl-alias']}")
    print(f"  other    : {counts['other']}")
    print(f"GAPS (no decoration) : {len(gaps)}")
    for e, r in gaps:
        print(f"    @{e.ordinal:<4} {e.name:<34} {r['note']}")
    print(f"EDGES (warn/alias)   : {len(edges)}")
    for e, r in edges:
        print(f"    @{e.ordinal:<4} {e.name:<34} -> {r['decorated']}  ({r['note']})")
    if table_path:
        print(f"\nfull table -> {table_path}")

    if args.emit_def:
        out = ["LIBRARY xboxkrnl.exe", "EXPORTS"]
        for e, r in rows:
            dec = r["decorated"]
            if dec is None:
                continue
            # An aliased export (RtlSnprintf = _snprintf) is imported by consumers
            # under its EXPORT name as a plain cdecl symbol (__imp__RtlSnprintf); the
            # `=target` only binds the kernel's own impl. Emit the export name.
            if r["conv"] == "cdecl-alias":
                dec = e.name
            # zig lib (/def) prepends a leading '_' to names WITHOUT an '@'
            # (i386 C decoration) and uses '@'-bearing names verbatim. So strip
            # one leading '_' from the underscore-only (data/cdecl/alias) names
            # and let the tool re-add it; emit stdcall/fastcall (_N@n / @N@n) as-is.
            if "@" not in dec and dec.startswith("_"):
                dec = dec[1:]
            # Data exports must be CONSTANT (IMPORT_OBJECT_CONST), not DATA: CONST
            # emits both __imp_X and a bare _X symbol, matching the blob. DATA emits
            # only __imp_X, so a non-dllimport ref to X auto-imports -> runtime
            # pseudo-reloc (breaks the MSVC-ABI libxnet link).
            flag = " CONSTANT" if r["conv"] == "data" else ""
            out.append(f"    {dec} @{e.ordinal} NONAME{flag}")
        args.emit_def.write_text("\n".join(out) + "\n")
        print(f"\nemitted .def -> {args.emit_def}  ({len(out) - 2} exports)")


if __name__ == "__main__":
    main()
