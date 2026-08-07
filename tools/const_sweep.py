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
    """Object-like macro names defined in a header (guards and function-like
    macros excluded)."""
    try:
        text = open(path, encoding='utf-8', errors='replace').read()
    except OSError:
        return set()
    # Strip block and line comments so a commented-out #define does not count.
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)

    out = set()
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
        out.add(name)
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
        names |= macros(p)
    return names


def main():
    ours = our_macros_union()

    real_rows, crt_rows, platform_rows = [], [], []
    for hp in sorted(glob.glob(os.path.join(XDK_INC, '**', '*.h'), recursive=True)):
        rel = os.path.relpath(hp, XDK_INC).replace('\\', '/')
        name = os.path.basename(hp)
        # Only compare headers we ship a counterpart for.
        our_h = _find_ours(name)
        if not our_h:
            continue
        defined = macros(hp)
        missing = sorted(n for n in defined
                         if n not in ours and n not in KNOWN_NON_GAPS)
        if not missing:
            continue
        row = (name, len(defined), missing)
        low = name.lower()
        if low in CRT_HEADERS or rel.lower() in CRT_HEADERS:
            crt_rows.append(row)
        elif low in PLATFORM_HEADERS:
            platform_rows.append(row)
        else:
            real_rows.append(row)

    real_rows.sort(key=lambda r: -len(r[2]))
    crt_rows.sort(key=lambda r: -len(r[2]))
    platform_rows.sort(key=lambda r: -len(r[2]))

    real_total = sum(len(r[2]) for r in real_rows)
    print(f"real 5849 constants absent from our headers: {real_total}\n")
    for name, total, missing in real_rows:
        print(f"{name:20s} {len(missing):3d} missing of {total}")
        for n in missing:
            print(f"    {n}")

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
