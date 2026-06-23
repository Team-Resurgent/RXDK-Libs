import re
import os

meson = open("vendor/picolibc/libc/stdio/meson.build", encoding="utf-8").read()
listed = set(re.findall(r"'([^']+\.c)'", meson))
dirp = "vendor/picolibc/libc/stdio"
allc = {f for f in os.listdir(dirp) if f.endswith(".c")}
extra = sorted(allc - listed)
print("extra count", len(extra))
for f in extra:
    print(f)
