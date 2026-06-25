#!/usr/bin/env python3
"""Copy libxapi Phase 1 tree into libs/libxapi/xapi/. Idempotent."""

from __future__ import annotations

import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIB = ROOT / "libs" / "libxapi"
XAPI = LIB / "xapi"
VENDOR = ROOT / "vendor" / "xbox_private" / "private"

PORT_RENAMES = {
    "xapi_intrlock.c": "intrlock.c",
    "xapi_fiber_switch.S": "fiber_switch.S",
    "xapi_muldiv.c": "muldiv.c",
    "xapi_rip.c": "rip.c",
    "xapi_compat.c": "compat.c",
    "xapi_kernel_patches.S": "kernel_patches.S",
    "xapi_findfirstset.c": "findfirstset.c",
    "xapi_tls_array.c": "tls_array.c",
    "xapi_lasterror.c": "lasterror.c",
    "xapi_tls.c": "tls.c",
    "xapi_fiber_tls.c": "fiber_tls.c",
    "xapi_kernel_import_ptrs.c": "kernel_import_ptrs.c",
    "xapi_tls_directory.c": "tls_directory.c",
    "xapi_tls_image.c": "tls_image.c",
    "xapi_usb_kernel_imports.c": "usb_kernel_imports.c",
    "xapi_usb_kernel_wrap.c": "usb_kernel_wrap.c",
    "xapi_usbd_pool.c": "usbd_pool.c",
}

SITE_RENAMES = {
    "xapi_site.h": "profile.h",
    "k32_bridge.h": "bridge_k32.h",
    "rtl_bridge.h": "bridge_rtl.h",
    "usb_bridge.h": "bridge_usb.h",
    "xapi_tls_layout.h": "tls_layout.h",
    "xapi_init_trace.h": "init_trace.h",
    "rxdk_kernel_import_ptrs.h": "kernel_import_ptrs.h",
}

INTERNAL_RENAMES = {
    "xapi.h": "compile.h",
    "xapip.h": "precompile.h",
    "xapi_xtl.h": "xtl.h",
    "xdk_bridge.h": "win32_bridge.h",
    "xapi_ntos_vendor.h": "ntos_vendor.h",
    "hal_data_bridge.h": "hal_data_bridge.h",
    "kpcrb_bridge.h": "kpcrb_bridge.h",
    "nt_bridge.h": "nt_bridge.h",
    "ntrtl.h": "ntrtl.h",
    "nturtl.h": "nturtl.h",
    "ntos.h": "ntos.h",
    "pe32.h": "pe32.h",
    "scsi.h": "scsi.h",
    "heap_rtl.h": "heap_rtl.h",
    "winnt_skipped.h": "winnt_skipped.h",
    "object_type_export.h": "object_type_export.h",
}

WIN32_SEED = [
    "windef.h",
    "winbase.h",
    "winerror.h",
    "xbox.h",
    "xkbd.h",
    "PshPack1.h",
    "PshPack2.h",
    "PshPack4.h",
    "PshPack8.h",
    "PopPack.h",
]

NT_SEED = [
    "nt.h",
    "ntdef.h",
    "ntstatus.h",
    "ntkeapi.h",
    "nti386.h",
    "ntdddisk.h",
    "ntddscsi.h",
    "ntddcdrm.h",
    "ntddcdvd.h",
    "ntddstor.h",
    "devioctl.h",
    "winioctl.h",
    "guiddef.h",
    "basetsd.h",
    "rpc.h",
    "rpcndr.h",
    "devguid.h",
    "excpt.h",
    "xtl.h",
    "rpcsal.h",
    "wtypes.h",
    "oaidl.h",
    "objidl.h",
    "oleidl.h",
]

VENDOR_DIRS = [
    (VENDOR / "ntos" / "xapi" / "k32", XAPI / "k32"),
    (VENDOR / "ntos" / "xapi" / "dll", XAPI / "dll"),
    (VENDOR / "ntos" / "rtl", XAPI / "rtl"),
    (VENDOR / "genx" / "types" / "uuid", XAPI / "uuid"),
    (VENDOR / "ntos" / "dd" / "usb" / "ohcd", XAPI / "usb" / "ohcd"),
    (VENDOR / "ntos" / "dd" / "usb" / "usbd", XAPI / "usb" / "usbd"),
    (VENDOR / "ntos" / "dd" / "usb" / "usbhub", XAPI / "usb" / "hub"),
    (VENDOR / "ntos" / "dd" / "usb" / "mu", XAPI / "usb" / "mu"),
    (VENDOR / "ntos" / "dd" / "usb" / "xid", XAPI / "usb" / "xid"),
    (VENDOR / "ntos" / "dd" / "usb" / "xmouse_dbg", XAPI / "usb" / "xid"),
    (VENDOR / "ntos" / "dd" / "usb" / "xidex", XAPI / "usb" / "xid"),
    (VENDOR / "inc", XAPI / "support" / "inc"),
    (VENDOR / "inc" / "crypto", XAPI / "support" / "crypto"),
    (VENDOR / "ntos" / "inc", XAPI / "support" / "inc" / "ntos"),
    (VENDOR / "ntos" / "fatx", XAPI / "support" / "fatx"),
    (VENDOR / "ntos" / "idex", XAPI / "support" / "idex"),
    (VENDOR / "ntos" / "dd" / "usb" / "inc", XAPI / "usb" / "inc"),
    (VENDOR / "ntos" / "xapi" / "inc", XAPI / "k32" / "inc"),
    (VENDOR / "ntos" / "rtl" / "inc", XAPI / "rtl" / "inc"),
]


def copy_tree(src: Path, dst: Path) -> None:
    if not src.exists():
        print(f"skip missing: {src}")
        return
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def copy_file(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def patch_text(text: str) -> str:
    reps = [
        (r'#include\s+"xapi_tls_layout\.h"', '#include "tls_layout.h"'),
        (r'#include\s+"xapi_init_trace\.h"', '#include "init_trace.h"'),
        (r'#include\s+"rxdk_kernel_import_ptrs\.h"', '#include "kernel_import_ptrs.h"'),
        (r'#include\s+<xapi/xapi\.h>', '#include "compile.h"'),
        (r'#include\s+<xapi/xapip\.h>', '#include "precompile.h"'),
        (r'#include\s+<xapi/xapi_xtl\.h>', '#include "xtl.h"'),
        (r'#include\s+<xapi/xdk_bridge\.h>', '#include "win32_bridge.h"'),
        (r'#include\s+<xapi/hal_data_bridge\.h>', '#include "hal_data_bridge.h"'),
        (r'#include\s+<xapi/kpcrb_bridge\.h>', '#include "kpcrb_bridge.h"'),
        (r'#include\s+<xapi/pe32\.h>', '#include "pe32.h"'),
        (r'#include\s+<xapi/heap_rtl\.h>', '#include "heap_rtl.h"'),
        (r'#include\s+<xapi/winnt_skipped\.h>', '#include "winnt_skipped.h"'),
        (r'#include\s+"xapi_ntos_vendor\.h"', '#include "ntos_vendor.h"'),
        (r'#include\s+"\.\.\\\.\.\\fatx\\', '#include "'),
        (r'#include\s+"\.\.\\\.\.\\idex\\', '#include "'),
        (r'#include\s+"\.\.\\fatx\\', '#include "'),
        (r'#include\s+"\.\.\\idex\\', '#include "'),
    ]
    for pat, repl in reps:
        text = re.sub(pat, repl, text)
    # Strip WINAPI/CALLBACK overrides from profile.h content
    if "RXDK_XAPI_SITE_H" in text or "RXDK_XAPI_SITE_H" in text:
        text = re.sub(
            r"/\* Clang x86-windows-gnu.*?\n#endif\n\n",
            "",
            text,
            flags=re.S,
        )
        text = re.sub(
            r"#ifdef __clang__\s*\n#undef WINAPI.*?#endif\n\n",
            "",
            text,
            flags=re.S,
        )
    return text


def patch_file(path: Path) -> None:
    if path.suffix not in {".c", ".cpp", ".h", ".S", ".asm"}:
        return
    raw = path.read_text(encoding="utf-8", errors="replace")
    new = patch_text(raw)
    if new != raw:
        path.write_text(new, encoding="utf-8", newline="\n")


def walk_patch(root: Path) -> None:
    if not root.exists():
        return
    for p in root.rglob("*"):
        if p.is_file():
            patch_file(p)


def copy_port() -> None:
    port = XAPI / "port"
    port.mkdir(parents=True, exist_ok=True)
    for src_name, dst_name in PORT_RENAMES.items():
        src = ROOT / "build" / src_name
        if src.exists():
            copy_file(src, port / dst_name)


def copy_site() -> None:
    site = XAPI / "site"
    site.mkdir(parents=True, exist_ok=True)
    gen = ROOT / "build" / "generated"
    for src_name, dst_name in SITE_RENAMES.items():
        src = gen / src_name
        if src.exists():
            text = src.read_text(encoding="utf-8", errors="replace")
            text = patch_text(text)
            (site / dst_name).write_text(text, encoding="utf-8", newline="\n")


def copy_internal() -> None:
    internal = XAPI / "internal"
    shims = internal / "shims"
    internal.mkdir(parents=True, exist_ok=True)
    shims.mkdir(parents=True, exist_ok=True)
    src_dir = ROOT / "include" / "xapi"
    for src_name, dst_name in INTERNAL_RENAMES.items():
        src = src_dir / src_name
        if src.exists():
            text = src.read_text(encoding="utf-8", errors="replace")
            text = patch_text(text)
            (internal / dst_name).write_text(text, encoding="utf-8", newline="\n")
    for shim in ["nt.h", "ntos.h", "ntrtl.h", "nturtl.h", "xtl.h", "excpt.h"]:
        src = ROOT / "include" / shim
        if src.exists():
            copy_file(src, shims / shim)


def copy_win32_nt() -> None:
    win32 = XAPI / "win32"
    nt = XAPI / "nt"
    win32.mkdir(parents=True, exist_ok=True)
    nt.mkdir(parents=True, exist_ok=True)
    xdk = ROOT / "include" / "xdk"
    sdk = ROOT / "include" / "sdk"
    for name in WIN32_SEED:
        src = xdk / name
        if src.exists():
            copy_file(src, win32 / name)
    # Copy crt excpt under nt/crt
    excpt_src = sdk / "crt" / "excpt.h"
    if excpt_src.exists():
        copy_file(excpt_src, nt / "crt" / "excpt.h")
    for name in NT_SEED:
        src = sdk / name
        if src.exists():
            copy_file(src, nt / name)
    # Expand: copy any sdk file referenced by nt.h chain (one level)
    copied = {p.name for p in nt.rglob("*") if p.is_file()}
    for name in list(copied):
        path = nt / name
        if not path.is_file():
            continue
        for m in re.finditer(r'#include\s*[<"]([^>"]+)[>"]', path.read_text(encoding="utf-8", errors="replace")):
            inc = m.group(1)
            if "/" in inc:
                sub = sdk / inc
            else:
                sub = sdk / inc
            if sub.exists() and sub.name not in copied:
                rel = inc.replace("/", "/")
                dst = nt / rel
                dst.parent.mkdir(parents=True, exist_ok=True)
                copy_file(sub, dst)
                copied.add(sub.name)


def emit_sources_zig() -> None:
    old = (ROOT / "build" / "xapi_sources.zig").read_text(encoding="utf-8")

    def repl_path(m: re.Match[str]) -> str:
        p = m.group(1)
        if p.startswith("build/xapi_"):
            base = Path(p).name
            new_name = PORT_RENAMES.get(base, base)
            return f'"libs/libxapi/xapi/port/{new_name}"'
        if p.startswith("vendor/xbox_private/private/"):
            rest = p[len("vendor/xbox_private/private/") :]
            mapping = [
                ("ntos/xapi/k32/", "xapi/k32/"),
                ("ntos/xapi/dll/", "xapi/dll/"),
                ("ntos/rtl/", "xapi/rtl/"),
                ("genx/types/uuid/", "xapi/uuid/"),
                ("ntos/dd/usb/ohcd/", "xapi/usb/ohcd/"),
                ("ntos/dd/usb/usbd/", "xapi/usb/usbd/"),
                ("ntos/dd/usb/usbhub/", "xapi/usb/hub/"),
                ("ntos/dd/usb/mu/", "xapi/usb/mu/"),
                ("ntos/dd/usb/xid/", "xapi/usb/xid/"),
                ("ntos/dd/usb/xmouse_dbg/", "xapi/usb/xid/"),
                ("ntos/dd/usb/xidex/", "xapi/usb/xid/"),
            ]
            for old_prefix, new_prefix in mapping:
                if rest.startswith(old_prefix):
                    rest = new_prefix + rest[len(old_prefix) :]
                    break
            return f'"libs/libxapi/{rest}"'
        return m.group(0)

    new = re.sub(r'"([^"]+)"', repl_path, old)
    header = "// Frozen libxapi manifest — do not regenerate from vendor NT sources.\n\n"
    (LIB / "sources.zig").write_text(header + new, encoding="utf-8", newline="\n")


def main() -> None:
    (LIB / "include").mkdir(parents=True, exist_ok=True)
    for sub in [
        "site",
        "win32",
        "nt",
        "internal",
        "k32",
        "dll",
        "rtl",
        "uuid",
        "usb",
        "port",
        "support",
        "minilib",
    ]:
        (XAPI / sub).mkdir(parents=True, exist_ok=True)

    for src, dst in VENDOR_DIRS:
        copy_tree(src, dst)

    copy_port()
    copy_site()
    copy_internal()
    copy_win32_nt()

    walk_patch(XAPI)

    emit_sources_zig()
    print(f"libxapi tree written under {LIB}")


if __name__ == "__main__":
    main()
