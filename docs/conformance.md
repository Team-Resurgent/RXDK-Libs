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

## Kit — runtime libc++ / C++23 tests

`samples/libcpp-smoke/tests.cpp` is a self-registering table (`RXDK_TEST_EQ` /
`RXDK_TEST_TRUE`) covering the C++23 surface. Build and run:

```powershell
.\scripts\compile.ps1 -Target libcpp-smoke -Iso
```

Expected serial output:

```
RXDK-LibsZig: libcpp-smoke start
RXDK-LibsZig libcpp-smoke OK passed=59 failed=0 total=59
```

Covered: exceptions (DWARF/Itanium), coroutines + `std::generator` (incl. recursive
`ranges::elements_of`), `thread_local` (emulated-TLS), `<format>`/`<print>`,
`<fstream>`/`<sstream>`, `<ranges>`, `<chrono>`, `<thread>`/`<mutex>` +
atomic-wait/`<latch>`/`<semaphore>`/`<barrier>`/`<stop_token>`, `<pmr>`,
`<valarray>`, `<charconv>` (incl. FP `to_chars`/`from_chars`), `<regex>`,
`<filesystem>`, `<any>`, `<mdspan>`, `<flat_map>`, `std::bind`, C11 `tss`.

### Known C++ gaps

| Feature | Status | Reason |
|---------|--------|--------|
| `<stdfloat>` (`float16_t`/`bfloat16_t`) | unsupported | clang has no `_Float16` on i386 without SSE2; target CPU is Pentium III (SSE1 only) |
| `<spanstream>`, `<stacktrace>` | unsupported | unimplemented in upstream libc++ itself (stacktrace also needs a symbolization backend) |
| `std::execution` parallel policies, `std::osyncstream`/`<syncstream>` | disabled | gated behind libc++'s experimental flag (`_LIBCPP_HAS_EXPERIMENTAL_PSTL` / experimental syncstream); left off by design |

Everything else in the present 72-header C++23 set compiles, links, and runs.

## Related samples

| Step | Artifact | Role |
|------|----------|------|
| `libc-smoke` | `libc-smoke.exe` | libc + C23 runtime matrix (incl. `<stdbit.h>`) |
| `libcpp-smoke` | `libcpp-smoke.exe` | libc++ / C++23 smoke (59 tests) |

Upstream stdtests autotools workflow (`configure && make`) can be used separately for host compiler matrices; RXDK integration starts with the manifest and grows runtime recipes here.
