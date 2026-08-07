"""Public 5849 object-like constants we do not define.

The API-gap audit asks "does this function exist"; this asks the quieter
question, "does this named value exist". A title that uses a 5849 #define we do
not carry fails to COMPILE, not link -- so unlike a missing API, a missing
constant is caught the moment a title's source references it. Still worth
tracking: it is the third leg (with the API and ownership audits) of "every lib
up to 5849", and it flushes out headers we adopted only partially.

What counts, and the traps that make a naive sweep lie (all three bit me once):

  * OBJECT-LIKE macros only. `#define FOO 1` counts; `#define BAR(x) ...` does
    not -- a function-like macro is API surface, measured by the api audit, and
    counting it here double-counts and mismeasures (its "value" is a body, not a
    constant).
  * NOT the include guard. `#ifndef _FOO_H_ / #define _FOO_H_` defines a name
    with no value; counting it reports every header as one constant short. We
    drop a bare `#define NAME` (no value) AND the classic guard shape.
  * "Defined ANYWHERE in ours" -- a constant 5849 keeps in header A may live in
    our header B (we consolidated a few). Compare against the union of all our
    public headers, exactly as the api audit compares against all our libs.

Only headers present in BOTH trees are compared (case-insensitively): a 5849
header we ship no counterpart for is a different kind of gap, not a constant we
forgot. Picolibc CRT headers (tchar.h, crtdbg.h, conio.h, ...) are OURS BY
CHOICE -- their "missing" constants are the intended state -- so they are listed
separately, below the real total, not folded into it.

    python tools/const_sweep.py
"""

import ast
import glob
import os
import re

XDK_INC = r"D:\Git\RXDK\POC\XDKSetup5849.17\XDK\xbox\include"
OUR_INC = r"D:\Git\RXDK-Libs\shared\include"

# CRT/POSIX headers where picolibc is our deliberate replacement: a 5849 constant
# absent here is intended, not a gap. Reported apart from the real total.
CRT_HEADERS = {
    'tchar.h', 'crtdbg.h', 'conio.h', 'mbstring.h', 'mbctype.h', 'direct.h',
    'io.h', 'process.h', 'share.h', 'sys/stat.h', 'sys/types.h', 'malloc.h',
    'new.h', 'typeinfo.h', 'use_ansi.h',
}

# Desktop Platform-SDK headers Microsoft shipped into the XDK essentially
# unchanged from Win32. We carry an Xbox-relevant SUBSET of each by design -- the
# Xbox kernel has no NT security model, no registry, no Itanium, no power
# management -- so their "missing" constants are overwhelmingly ours-by-choice,
# not gaps. WinNT.h alone accounts for 900+ (ACL/token/SID access masks, PE image
# directory entries, EMARCH_ENC_* Itanium encodings, ES_* execution-state), which
# would bury the real Xbox-SDK signal. Reported apart from the real total. Note:
# a small inherited header with a handful of gaps (WinDef.h) is a specific
# omission, not wholesale subsetting, so it stays in the real bucket -- only the
# wholesale-subset headers are listed here.
PLATFORM_HEADERS = {
    'winnt.h', 'winbase.h', 'winuser.h', 'wingdi.h', 'winreg.h', 'winnls.h',
    'winerror.h', 'windows.h', 'wincon.h', 'winver.h', 'winsvc.h', 'imagehlp.h',
}

# Value differences that are INTENTIONALLY ours, verified individually -- our
# value is deliberate, not a bug, so the value pass must not flag them. Keyed by
# (header, macro).
KNOWN_VALUE_DIVERGENCE = {
    # Our on-disk wave-bank format is the leak TRUNK's version 2 (the Mar-02
    # snapshot and 5849 shipped version 1); xactbld emits it and libxact's
    # wavebank.cpp rejects any other version, and the header's szBankName field
    # length is part of that struct layout. It is a closed tool<->engine contract,
    # not a title-facing ABI (titles never parse .xwb at runtime), so matching
    # 5849 here would mean reformatting our banks for no caller's benefit.
    ('wavbndlr.h', 'WAVEBANKHEADER_VERSION'),
    ('wavbndlr.h', 'WAVEBANKHEADER_BANKNAME_LENGTH'),
}

# Headers whose ENUM members are an internal implementation detail, not a
# title-facing ABI, so their values legitimately differ from 5849 and should not
# be value-compared (their #defines still are). d3d8perf.h's D3DAPI_INDEX is the
# profiler's per-API counter-slot table: libd3d8 indexes its own
# g_PerfCounters.m_APICounters[] array by these, sized to D3DAPI_MAX -- fully
# internal and self-consistent. 5849 simply has ~15 more entries because its D3D
# gained methods; matching it would renumber 180+ slots (and instrument those
# call sites) for a debug-only profiler no host tool in this project consumes.
INTERNAL_ENUM_HEADERS = {'d3d8perf.h'}

# Names the sweep would flag that are NOT real gaps -- verified individually, so
# they do not reappear as noise every run. Keep the reason with each.
KNOWN_NON_GAPS = {
    # A function alias, not a constant: #define CompileShader XGCompileShader.
    # We lack the FUNCTION (api audit's job); aliasing here would point at an
    # absent symbol and turn a clear gap into a puzzling link error.
    'CompileShader',
    # These reach us from guiddef.h, just not from the header the sweep found
    # them in.
    'REFGUID', 'DECLSPEC_SELECTANY', '_REFGUID_DEFINED',
}


def macros(path):
    """Object-like macros of a header as {name: replacement-text} (guards and
    function-like macros excluded)."""
    try:
        text = open(path, encoding='utf-8', errors='replace').read()
    except OSError:
        return {}
    # Strip block and line comments so a commented-out #define does not count.
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)

    out = {}
    guard = _guard_name(text)
    for m in re.finditer(r'(?m)^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)(.?)', text):
        name, nextch = m.group(1), m.group(2)
        if nextch == '(':          # function-like macro -- not a constant
            continue
        if name == guard:          # the include guard -- not a constant
            continue
        # A bare `#define NAME` with no replacement list is a flag/guard, not a
        # value. Require something non-empty after the name on the line.
        rest = _rest_of_define(text, m.end() - 1)
        if not rest.strip():
            continue
        out[name] = rest.strip()
    return out


def _guard_name(text):
    """The header-guard macro, or None.

    ONLY the top-of-file guard counts: the first `#define`/`#ifndef` directive in
    the file must be `#ifndef X` immediately followed by `#define X`. A whole-file
    scan for any `#ifndef X\n#define X` pair is wrong -- the common overridable-
    default idiom `#ifndef STRICT / #define STRICT 1` (mid-file, and a real valued
    constant) matches that shape and would be silently dropped. Anchoring to the
    first directive fixes it: a header using `#pragma once` + `#define _FOO_` has
    no `#ifndef`-guard, so STRICT is correctly counted.
    """
    for m in re.finditer(r'(?m)^[ \t]*#[ \t]*(ifndef|define)[ \t]+([A-Za-z_]\w*)', text):
        kind, name = m.group(1), m.group(2)
        if kind == 'define':
            return None  # first directive is a #define, not an #ifndef guard
        # kind == 'ifndef': is it immediately followed by `#define <same name>`?
        after = text[m.end():]
        d = re.match(r'\s*#[ \t]*define[ \t]+' + re.escape(name) + r'\b', after)
        return name if d else None
    return None


def _rest_of_define(text, pos):
    """Text after the macro name on its (line-continued) logical line."""
    line = []
    i = pos
    while i < len(text):
        c = text[i]
        if c == '\\' and i + 1 < len(text) and text[i + 1] == '\n':
            i += 2
            continue
        if c == '\n':
            break
        line.append(c)
        i += 1
    return ''.join(line)


def our_macros_union():
    names = set()
    for p in glob.glob(os.path.join(OUR_INC, '**', '*.h'), recursive=True):
        names |= set(macros(p))
    return names


# --- value reduction, for the "defined but WRONG" pass ---------------------------
#
# A constant we define with the wrong value is worse than one we omit: it compiles
# and misbehaves. But comparing replacement TEXT is hopeless (`0x0004` vs `4`,
# `((USHORT)2)` vs `2`, whitespace), and comparing evaluated values needs a
# preprocessor. The precise-but-partial answer: reduce only what reduces to an
# integer WITHOUT expanding any identifier or macro call, and compare those. A
# value that still contains a name (`DSBCAPS_MUTE3DATMAXDISTANCE`, `XBDM_HRESERR(23)`)
# is skipped, not guessed -- so this reports real numeric disagreements and stays
# silent where it cannot be sure. Recall is sacrificed for zero false positives.

_CAST = re.compile(
    r'\(\s*(?:unsigned\s+|signed\s+|const\s+)*'
    r'(?:int|long|short|char|USHORT|UINT|DWORD|ULONG|LONG|WORD|BYTE|INT|SHORT|'
    r'UCHAR|ULONGLONG|LONGLONG|size_t|SIZE_T|DWORD_PTR)\s*\)', re.I)


def reduce_int(text):
    """The integer a replacement text denotes, or None if it is not a pure
    numeric/bitwise expression (any identifier, float, comma, or / % → None)."""
    t = text.strip().replace('\\', ' ')
    if not t or ',' in t:
        return None                      # initializer list, not a scalar
    if re.search(r'\.\d|\d\.|\bf\b|\d[fF]\b', t):
        return None                      # float
    prev = None
    while prev != t:                     # peel nested casts
        prev, t = t, _CAST.sub('', t)
    t = re.sub(r'\b(0[xX][0-9a-fA-F]+|\d+)[uUlL]+\b', r'\1', t)  # drop U/L suffix
    if '/' in t or '%' in t:
        return None                      # avoid C-vs-Python division semantics
    if not re.fullmatch(r'[0-9a-fA-FxX+\-*<>|&^~() \t]*', t):
        return None                      # a leftover identifier -- cannot be sure
    try:
        return _safe_eval(ast.parse(t.strip(), mode='eval').body)
    except (SyntaxError, ValueError, TypeError):
        return None


def _safe_eval(node):
    import ast as _ast
    if isinstance(node, _ast.Constant) and isinstance(node.value, int):
        return node.value
    if isinstance(node, _ast.Num) and isinstance(node.n, int):  # <3.8
        return node.n
    if isinstance(node, _ast.UnaryOp) and isinstance(node.op, (_ast.USub, _ast.UAdd, _ast.Invert)):
        v = _safe_eval(node.operand)
        return {_ast.USub: -v, _ast.UAdd: v, _ast.Invert: ~v}[type(node.op)]
    if isinstance(node, _ast.BinOp):
        a, b = _safe_eval(node.left), _safe_eval(node.right)
        ops = {_ast.Add: a + b, _ast.Sub: a - b, _ast.Mult: a * b,
               _ast.LShift: a << b if 0 <= b < 64 else None,
               _ast.RShift: a >> b if 0 <= b < 64 else None,
               _ast.BitOr: a | b, _ast.BitAnd: a & b, _ast.BitXor: a ^ b}
        if type(node.op) in ops:
            return ops[type(node.op)]
    raise ValueError('unsupported')


def _reduce_with_syms(text, syms):
    """reduce_int, but first substitute any known symbol (prior enum members) by
    its integer value. Lets `B = A + 1` resolve; an unknown name still → None."""
    t = re.sub(r'[A-Za-z_]\w*',
               lambda m: str(syms[m.group()]) if m.group() in syms else m.group(),
               text)
    return reduce_int(t)


_ENUM = re.compile(r'\benum\b[^{;]*\{([^{}]*)\}', re.S)


def enum_values(path):
    """{member: value} for enum members that resolve to an integer.

    Enum members are C's other named-constant mechanism, and 5849 inserting one
    mid-list shifts every following value -- the same silent-miscompile risk as a
    wrong #define. Values are tracked sequentially (implicit member = prev + 1),
    with `= expr` reduced after substituting earlier members; once a member fails
    to resolve, the running counter is dropped so nothing downstream is guessed.
    """
    try:
        text = open(path, encoding='utf-8', errors='replace').read()
    except OSError:
        return {}
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)

    out = {}
    for body in (m.group(1) for m in _ENUM.finditer(text)):
        cur = -1                              # first implicit member is 0
        for member in _split_top(body):
            member = member.strip()
            if not member:
                continue
            mm = re.match(r'([A-Za-z_]\w*)\s*(?:=(.*))?$', member, re.S)
            if not mm:
                cur = None
                continue
            name, expr = mm.group(1), mm.group(2)
            if expr is not None:
                v = _reduce_with_syms(expr, out)
                cur = v                       # None if unresolved -> stop counting
            else:
                cur = cur + 1 if cur is not None else None
            if cur is not None:
                out[name] = cur
    return out


def _split_top(body):
    """Split an enum body on commas that are not inside parentheses."""
    parts, depth, cur = [], 0, []
    for ch in body:
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        if ch == ',' and depth == 0:
            parts.append(''.join(cur)); cur = []
        else:
            cur.append(ch)
    parts.append(''.join(cur))
    return parts




def _header_values(path, include_enums=True):
    """{name: reduced-int} for one header -- #defines and (optionally) enum
    members together."""
    vals = {}
    for n, raw in macros(path).items():
        r = reduce_int(raw)
        if r is not None:
            vals[n] = r
    if include_enums:
        vals.update(enum_values(path))
    return vals


def main():
    ours = our_macros_union()

    real_rows, crt_rows, platform_rows = [], [], []
    mismatches = []   # (header, name, ours, theirs) -- defined-but-wrong
    for hp in sorted(glob.glob(os.path.join(XDK_INC, '**', '*.h'), recursive=True)):
        rel = os.path.relpath(hp, XDK_INC).replace('\\', '/')
        name = os.path.basename(hp)
        # Only compare headers we ship a counterpart for.
        our_h = _find_ours(name)
        if not our_h:
            continue
        low = name.lower()
        ours_by_choice = (low in CRT_HEADERS or rel.lower() in CRT_HEADERS
                          or low in PLATFORM_HEADERS)

        defined = macros(hp)
        missing = sorted(n for n in defined
                         if n not in ours and n not in KNOWN_NON_GAPS)
        if missing:
            row = (name, len(defined), missing)
            if low in CRT_HEADERS or rel.lower() in CRT_HEADERS:
                crt_rows.append(row)
            elif low in PLATFORM_HEADERS:
                platform_rows.append(row)
            else:
                real_rows.append(row)

        # Value-mismatch pass: only the Xbox-SDK headers, only names that reduce
        # to an integer on BOTH sides. Covers #define constants and enum members.
        # Compared against OUR SAME-NAMED header, not a global union -- enum member
        # names are not globally unique (dsstdfx.h and dmusicfx.h both name an
        # I3DL2Reverb node at different graph positions), so a global map reports
        # phantom mismatches from a same-named member in an unrelated header.
        if not ours_by_choice:
            use_enums = low not in INTERNAL_ENUM_HEADERS
            ours_here = _header_values(our_h, use_enums)
            for n, theirs in _header_values(hp, use_enums).items():
                if n in KNOWN_NON_GAPS or (name, n) in KNOWN_VALUE_DIVERGENCE:
                    continue
                if n in ours_here and theirs != ours_here[n]:
                    mismatches.append((name, n, ours_here[n], theirs))

    real_rows.sort(key=lambda r: -len(r[2]))
    crt_rows.sort(key=lambda r: -len(r[2]))
    platform_rows.sort(key=lambda r: -len(r[2]))

    real_total = sum(len(r[2]) for r in real_rows)
    print(f"real 5849 constants absent from our headers: {real_total}\n")
    for name, total, missing in real_rows:
        print(f"{name:20s} {len(missing):3d} missing of {total}")
        for n in missing:
            print(f"    {n}")

    print(f"\nconstants we define with a DIFFERENT value than 5849: {len(mismatches)}")
    for header, n, mine, theirs in sorted(mismatches):
        print(f"    {header}: {n} = {mine} (ours) vs {theirs} (5849)")

    crt_total = sum(len(r[2]) for r in crt_rows)
    plat_total = sum(len(r[2]) for r in platform_rows)
    print(f"\n(CRT/picolibc headers, ours by choice: {crt_total} across "
          f"{len(crt_rows)} headers -- not counted)")
    print(f"(inherited Win32 platform headers, ours by choice: {plat_total} across "
          f"{len(platform_rows)} headers -- not counted: "
          f"{', '.join(r[0] for r in platform_rows)})")


def _find_ours(name):
    for p in glob.glob(os.path.join(OUR_INC, '**', '*.h'), recursive=True):
        if os.path.basename(p).lower() == name.lower():
            return p
    return None


if __name__ == '__main__':
    main()
