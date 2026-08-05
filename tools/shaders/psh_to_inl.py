"""Assemble a .psh into the D3DPIXELSHADERDEF .inl form the XDK samples #include.

xsasm's default output (.xpu) is the four-byte tag "PSB0" followed by a verbatim
D3DPIXELSHADERDEF -- verified byte-for-byte against all 11 shipped .psh/.inl
pairs in FocusBlur and Fur, so the words below need no interpretation, only
naming. The shipped files spell their values with the PS_* packing macros; these
carry the same words as hex, which the compiler cannot tell apart.
"""
import subprocess, struct, os, sys

# Set XSASM to point at your copy; the default is where the 5849 XDK installs it.
XS = os.environ.get("XSASM", r"D:\Git\RXDK\POC\XDKSetup5849.17\XDK\xbox\bin\xsasm.exe")

# D3DPIXELSHADERDEF, in declaration order (shared/include/d3d8types.h).
FIELDS = [
    ("PSAlphaInputs", 8), ("PSFinalCombinerInputsABCD", 1), ("PSFinalCombinerInputsEFG", 1),
    ("PSConstant0", 8), ("PSConstant1", 8), ("PSAlphaOutputs", 8), ("PSRGBInputs", 8),
    ("PSCompareMode", 1), ("PSFinalCombinerConstant0", 1), ("PSFinalCombinerConstant1", 1),
    ("PSRGBOutputs", 8), ("PSCombinerCount", 1), ("PSTextureModes", 1), ("PSDotMapping", 1),
    ("PSInputTexture", 1), ("PSC0Mapping", 1), ("PSC1Mapping", 1), ("PSFinalCombinerConstants", 1),
]


def assemble(psh, xpu):
    if os.path.exists(xpu):
        os.remove(xpu)
    r = subprocess.run([XS, psh, xpu], capture_output=True, text=True)
    if not os.path.exists(xpu):
        raise SystemExit("xsasm failed on %s: %s" % (psh, (r.stdout or r.stderr).strip()))
    w = struct.unpack("<61I", open(xpu, "rb").read())
    if w[0] != 0x30425350:
        raise SystemExit("%s: unexpected tag %08x" % (xpu, w[0]))
    return w[1:]


def emit(words, psh_name):
    out = ["// Xbox Shader Assembler 1.00.5849.1",
           "// Generated from %s -- regenerate with: xsasm %s out.xpu" % (psh_name, psh_name),
           "D3DPIXELSHADERDEF psd;",
           "ZeroMemory(&psd, sizeof(psd));"]
    i = 0
    for name, n in FIELDS:
        if n == 1:
            if words[i]:
                out.append("psd.%s=0x%08x;" % (name, words[i]))
        else:
            for k in range(n):
                if words[i + k]:
                    out.append("psd.%s[%d]=0x%08x;" % (name, k, words[i + k]))
        i += n
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    sample_dir = sys.argv[1]
    for psh in sorted(os.listdir(os.path.join(sample_dir, "Media", "Shaders"))):
        if not psh.lower().endswith(".psh"):
            continue
        base = os.path.splitext(psh)[0]
        src = os.path.join(sample_dir, "Media", "Shaders", psh)
        words = assemble(src, os.path.abspath(base + ".gen.xpu"))
        dest = os.path.join(sample_dir, base + ".inl")
        with open(dest, "w", newline="\r\n") as f:
            f.write(emit(words, psh))
        print("wrote", dest)
