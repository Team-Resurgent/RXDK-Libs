# RXDK-Libs tests

Small RXDK titles that verify library findings on real hardware / xemu — the
place to turn a claim ("does X actually work?") into a runnable PASS/FAIL check.
Each test is a normal RXDK project (`.vcxproj` + `rxdk.project.json`) that builds
an `.xbe`, prints results with `DbgPrint`, and parks. Open `RxdkTests.sln` in
Visual Studio (with the RXDK extension) or open a test folder in VS Code.

## Running a test

1. **Build the libs the test links against, and stage them into the SDK** the
   project resolves (`C:\ProgramData\RXDK\sdk\lib\{debug,release}`), so the test
   exercises *current* source rather than the last-published SDK:

   ```bash
   ./build.ps1                 # from the repo root: builds dist\lib + dist\include
   # then copy dist\lib\{debug,release}\*.lib and dist\include\* into
   # C:\ProgramData\RXDK\sdk\  (or use the extension's "Fetch Latest RXDK-SDK"
   # once a change is published)
   ```

2. **Build + deploy + run** the test: open `RxdkTests.sln`, pick `Debug|Xbox`,
   build, then Deploy to your Xbox / launch in xemu. Or from VS Code, open the
   test folder and use RXDK: Build / Deploy / Run.

3. **Read the results** on the debug monitor (xbWatson) — each test prints
   `PASS`/`FAIL` lines and a `pass=N fail=N` summary.

## Tests

| Project | Finding under test |
|---------|--------------------|
| `ThreadTimeouts` | C11 `<threads.h>` timed primitives: `mtx_timedlock` and `cnd_timedwait` actually honor their deadline (return `thrd_timedout`) instead of blocking forever. |
| `Exceptions` | C++ DWARF/Itanium exceptions: throw across a frame, destructor runs during unwind, catch by type, `what()`, rethrow. Requires `"exceptions": true` (see below). |

## Findings so far

- **C++ exceptions are opt-in, not broken.** Titles build `-fno-exceptions` by
  default (correct for a 64 MB console), so `try`/`throw` won't even compile
  unless the project sets `"exceptions": true` in `rxdk.project.json`
  (`<RxdkExceptions>true</RxdkExceptions>` in the `.vcxproj`) → `-fexceptions`.
  The EH runtime (libunwind + `.eh_frame` markers) is linked either way, so with
  the flag on, EH code compiles and links cleanly. The `Exceptions` test then
  confirms it at runtime on hardware.
- The title compile line also confirms `-march=pentium3` (MMX/SSE1, no SSE2) and
  `-fno-builtin` — both real, both by design.

## Adding a test

Copy `ThreadTimeouts/` to a new folder, rename the three files, give the
`.vcxproj`/`.sln` a fresh `ProjectGuid`, pick a unique `RxdkTestId`, write the
check in `main.c` (report with `DbgPrint`, park with an infinite sleep at the
end), and add the project to `RxdkTests.sln`. Keep tests dependency-light — link
only the libraries the finding needs (`libxbdm` gives you the debug-monitor
connection for xbWatson).
