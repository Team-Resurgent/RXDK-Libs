import subprocess, struct, os, glob, sys

XS  = os.environ.get("XSASM", r"D:\Git\RXDK\POC\XDKSetup5849.17\XDK\xbox\bin\xsasm.exe")
INC = r"D:\Git\RXDK-Libs\shared\include"
BS  = chr(92)  # backslash

def check(psh, inl):
    base = os.path.splitext(os.path.basename(psh))[0]
    xpu = os.path.abspath(base + ".gen.xpu")
    if os.path.exists(xpu):
        os.remove(xpu)
    r = subprocess.run([XS, psh, xpu], capture_output=True, text=True)
    if not os.path.exists(xpu):
        return "ASM-FAIL " + (r.stdout or r.stderr).strip()[:120]
    exe = os.path.abspath("d_%s.exe" % base)
    c = subprocess.run(["zig", "cc", "-fno-sanitize=undefined", "-I", INC,
                        '-DSHADER_INL="%s"' % inl.replace(BS, "/"),
                        "-o", exe, "dump_psd.c"], capture_output=True, text=True)
    if not os.path.exists(exe):
        return "CC-FAIL " + " | ".join(c.stderr.strip().splitlines()[:2])
    shipped = [int(x, 16) for x in subprocess.run([exe], capture_output=True, text=True).stdout.split()]
    b = open(xpu, "rb").read()
    gen = list(struct.unpack("<%dI" % (len(b) // 4), b))
    if gen[0] != 0x30425350:
        return "BAD-MAGIC %08x" % gen[0]
    if len(gen) != 61:
        return "BAD-SIZE %d dwords" % len(gen)
    diff = [i for i in range(60) if shipped[i] != gen[1 + i]]
    return None if not diff else "DIFF words %r" % diff[:8]

ok = fail = 0
for d in [r"D:\Git\RXDK-VS20XX\XDKSamples\Graphics\FocusBlur",
          r"D:\Git\RXDK-VS20XX\XDKSamples\Graphics\Fur"]:
    for psh in sorted(glob.glob(os.path.join(d, "Media", "Shaders", "*.psh"))):
        base = os.path.splitext(os.path.basename(psh))[0]
        inl = os.path.join(d, base + ".inl")
        if not os.path.exists(inl):
            continue
        err = check(psh, inl)
        if err is None:
            print("MATCH ", base); ok += 1
        else:
            print("FAIL  ", base, err); fail += 1
print("=== ok=%d fail=%d" % (ok, fail))
