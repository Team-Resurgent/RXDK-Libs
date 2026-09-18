#!/usr/bin/env python3
"""
Generate a single combined RXDK-Libs title from the ported RXDK-360 stdlib suite.

Each tests/stdlib/t_<name>.{c,cpp} keeps its own `int main()`. For each we emit a
shim that renames it (`#define main t_<name>`) so it becomes an ordinary
function, plus a driver.cpp that calls every section in turn bracketed by a
`[T] SECT <name>` marker, and an rxdk.project.json listing them all. One title,
one xemu boot; the serial log's [T] SECT/PASS/FAIL/DONE lines are parsed by
run_suite.py.

Usage: python gen_suite.py            # all units
       python gen_suite.py --config Debug|Release
Edit EXCLUDE below to skip a unit that does not yet compile/link on the OG libs.
"""
import argparse, glob, os

HERE = os.path.dirname(os.path.abspath(__file__))
GEN = os.path.join(HERE, "build")

# Units that do not yet compile/link against the current OG-Xbox libs; kept out of
# the combined title so the rest still run. Goal is to empty this list.
# Units that do not yet compile against the installed SDK. Split by cause:
#  - C++23 features present in the CURRENT vendored libc++ but not the installed
#    v1.2.3 SDK headers (spanstream/stacktrace/cartesian/chunk_slide/move_only_fn)
#  - genuine OG libc gaps (realpath/pread/free_sized/syslog.h/sigaction/sigval/pthread)
EXCLUDE = {
    "t_c23lang", "t_cwd", "t_posix", "t_posix2", "t_posix3", "t_posix4",
    "t_posix5", "t_posixext", "t_posixglob", "t_posixstub", "t_posixun",
    "t_pthread", "t_signal", "t_syslog", "t_timer",
    "t_cartesian", "t_chunk_slide", "t_move_only_fn", "t_spanstream", "t_stacktrace",
    # link-blocked on OG-runtime glue not yet ported (360-specific):
    #   t_args     -> __rxdk_parse_cmdline (360 crt_start.c argv parsing)
    #   t_mscompat -> _beginthreadex/ExCreateThread/NtClose (360 thread kernel),
    #                 __CxxFrameHandler (MSVC EH), std::_Lockit (MSVC STL locks)
    "t_args", "t_mscompat",
}

DEBUG_LIBS = ["libxbdmd.lib", "libxapid.lib", "libkerneld.lib",
              "libcd.lib", "libcppd.lib", "libcompatd.lib"]
RELEASE_LIBS = ["libxapi.lib", "libkernel.lib", "libc.lib", "libcpp.lib", "libcompat.lib"]


def units():
    out = []
    for path in sorted(glob.glob(os.path.join(HERE, "t_*.c")) +
                       glob.glob(os.path.join(HERE, "t_*.cpp"))):
        base = os.path.basename(path)
        name = base[:base.rfind(".")]        # t_string_view
        if name in EXCLUDE:
            continue
        ext = base[base.rfind("."):]          # .c / .cpp
        out.append((name, ext, base))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default="Debug")
    args = ap.parse_args()
    os.makedirs(GEN, exist_ok=True)

    us = units()
    srcs = []

    # per-unit shims: rename main -> t_<name>, pull in the test verbatim
    for name, ext, base in us:
        shim = "shim_%s%s" % (name, ext)
        with open(os.path.join(GEN, shim), "w", newline="\n") as f:
            f.write("/* generated shim: %s becomes a callable section */\n" % base)
            f.write("#define main %s\n" % name)
            f.write('#include "../%s"\n' % base)
            f.write("#undef main\n")
        srcs.append(shim)

    # driver: declare every section (C tests keep C linkage) and call them
    c_units = [n for n, e, _ in us if e == ".c"]
    cpp_units = [n for n, e, _ in us if e == ".cpp"]
    with open(os.path.join(GEN, "driver.cpp"), "w", newline="\n") as f:
        f.write("/* generated combined-suite driver */\n")
        f.write('extern "C" int DbgPrint(const char *fmt, ...);\n\n')
        if c_units:
            f.write('extern "C" {\n')
            for n in c_units:
                f.write("    int %s(void);\n" % n)
            f.write("}\n")
        for n in cpp_units:
            f.write("int %s(void);\n" % n)
        f.write("\nstruct Section { const char *name; int (*fn)(void); };\n")
        f.write("static const Section kSections[] = {\n")
        for n, _e, _b in us:
            f.write('    { "%s", %s },\n' % (n[2:], n))   # strip leading t_
        f.write("};\n\n")
        f.write("int main(void)\n{\n")
        f.write('    DbgPrint("========== RXDK-Libs stdlib suite ==========\\n");\n')
        f.write("    for (const auto &s : kSections) {\n")
        f.write('        DbgPrint("[T] SECT %s\\n", s.name);\n')
        f.write("        s.fn();\n")
        f.write("    }\n")
        f.write('    DbgPrint("[T] ALLDONE\\n");\n')
        f.write("    return 0;\n}\n")

    # manifest
    libs = DEBUG_LIBS if args.config == "Debug" else RELEASE_LIBS
    cfgkey = args.config
    cfgval = "debug" if args.config == "Debug" else "release"
    src_json = ",\n".join('        "%s"' % s for s in ["driver.cpp"] + srcs)
    lib_json = ",\n".join('        "%s"' % l for l in libs)
    manifest = '''{
  "name": "RxdkStdlib",
  "defaultConfiguration": "%s",
  "configurations": {
    "%s": {
      "configuration": "%s",
      "exceptions": true,
      "sources": [
%s
      ],
      "libraries": [
%s
      ],
      "imageBuild": {
        "stackSize": 262144,
        "debug": true,
        "noLogo": true,
        "noLibWarn": true,
        "dontMountUtilityDrive": false,
        "testId": "0xffff2001",
        "testName": "RxdkStdlib",
        "testVersion": "4096"
      },
      "createIso": true,
      "outputDir": "out\\\\%s"
    }
  }
}
''' % (cfgkey, cfgkey, cfgval, src_json, lib_json, cfgkey)
    with open(os.path.join(GEN, "rxdk.project.json"), "w", newline="\n") as f:
        f.write(manifest)

    print("generated %d sections (%d C, %d C++) into %s" % (len(us), len(c_units), len(cpp_units), GEN))
    if EXCLUDE:
        print("excluded:", ", ".join(sorted(EXCLUDE)))


if __name__ == "__main__":
    main()
