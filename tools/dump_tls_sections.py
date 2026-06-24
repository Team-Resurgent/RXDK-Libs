#!/usr/bin/env python3
import glob, os, subprocess, struct, sys

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
rsp = os.path.join(root, "zig-out/lib/libxapi_core.rsp")
objs = []
if os.path.isfile(rsp):
    for line in open(rsp):
        line = line.strip().strip('"')
        if line.endswith(".obj"):
            objs.append(line)
for p in glob.glob(os.path.join(root, "zig-out/obj/**/*.obj"), recursive=True):
    objs.append(p)
for p in glob.glob(os.path.join(root, "zig-out/samples/**/*.obj"), recursive=True):
    objs.append(p)

seen = {}
for obj in objs:
    if not os.path.isfile(obj):
        continue
    try:
        out = subprocess.check_output(["llvm-objdump", "-h", obj], text=True, stderr=subprocess.DEVNULL)
    except Exception:
        continue
    for line in out.splitlines():
        if ".tls" not in line.lower():
            continue
        parts = line.split()
        if len(parts) < 2:
            continue
        name = parts[1]
        if name not in seen:
            seen[name] = os.path.basename(obj)

for name in sorted(seen):
    print(f"{name}\t{seen[name]}")
