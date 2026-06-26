# C / libc conformance (kit + host)

Runtime tests live in `samples/conformance/c/` and follow the header taxonomy from [winspool/stdtests](https://github.com/winspool/stdtests) (`vendor/stdtests/template/c_header.txt`).

## Host — header availability

Compile-only check against RXDK include paths (no autotools required):

```powershell
.\scripts\run-stdtests-headers.ps1
python tools\check_c_headers.py --report zig-out\conformance\headers.tsv
```

Uses `vendor/stdtests/template/c_header.txt` as the manifest. Skips headers not expected on kit yet (`threads.h`, `signal.h`, …).

## Kit — runtime libc tests

1. Add or edit a test body in `tools/conformance_recipes.py`.
2. Regenerate and build:

```powershell
python tools/generate_conformance_tests.py
zig build libc-smoke
.\scripts\compile.ps1 -Target libc-smoke -Iso
```

Expected serial output:

```
RXDK-LibsZig: libc-smoke start
RXDK-LibsZig libc-smoke OK passed=16 failed=0 total=16
```

## Related samples

| Step | Artifact | Role |
|------|----------|------|
| `libc-smoke` | `libc-smoke.exe` | libc + C23 runtime matrix (incl. `<stdbit.h>`) |
| `libcpp-smoke` | `libcpp-smoke.exe` | libc++ / C++23 smoke |

Upstream stdtests autotools workflow (`configure && make`) can be used separately for host compiler matrices; RXDK integration starts with the manifest and grows runtime recipes here.
