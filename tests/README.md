# RXDK-Libs tests

`RxdkTests` is one RXDK title that runs every library check and prints a
`PASS`/`FAIL` line per check — so a single boot on xemu or real hardware verifies
the lot. It's the place to turn a claim ("does X actually work?") into a runnable
result. Open `RxdkTests.sln` in Visual Studio (with the RXDK extension) or open
the `RxdkTests/` folder in VS Code.

## What it checks

| Section | Verifies |
|---------|----------|
| C11 thread timeouts | `mtx_timedlock` / `cnd_timedwait` honor their deadline (`thrd_timedout`) instead of blocking forever. |
| Relative paths | default cwd is `D:\` (title dir), and relative `fopen`/`stat` resolve there (a deployed `probe.dat`). |
| MSVC string format | `%S`/`%C`/`%I64d` are translated through the bounded `snprintf`/`vsnprintf` (SDL's `SDL_vsnprintf` path). |
| C++ exceptions | throw across a frame, destructor on unwind, catch-by-type, `what()`, rethrow. Needs `"exceptions": true`. |

## Running it

**Build the libs it links, and stage them into the SDK** the project resolves
(`C:\ProgramData\RXDK\sdk\lib\{debug,release}`), so the test exercises *current*
source. For a single lib: `zig build libc -Doptimize=Debug` then copy
`zig-out\lib\libc.lib` over the SDK's; for a full refresh, `build.ps1` +
`scripts\publish-sdk.ps1`.

**In VS / VS Code:** build `RxdkTests`, then Deploy to your Xbox / Launch in
xemu, and read the `DbgPrint` output on xbWatson.

**On xemu from the command line** (devkit build, so `DbgPrint` reaches the
serial port): point the DVD at the ISO and route serial to stdout —

```bash
# from the xemu working dir (portable config with relative rom paths):
#   set dvd_path in xemu.toml to <...>\RxdkTests\out\Debug\XISO\RxdkTests.iso
./xemu.exe -device lpc47m157 -serial stdio
```

The `-device lpc47m157 -serial stdio` pair is what surfaces `DbgPrint`; without
it the debug output only shows in xemu's own debug view.

## Findings so far

- **Relative `fopen`/`stat` now resolve against the title dir.** `fopen`/`stat`
  go through libc's `nt_open` (fileio.c), which used to skip the cwd entirely, so
  a relative path became `\??\<name>` with no drive and failed — independent of
  the cwd default. `nt_open` now shares `__rxdk_norm_path` (dirio.c), so
  `fopen("data/x.dat")` reaches `D:\data\x.dat`. Verified on xemu.
- **C++ exceptions compile and link but do NOT work at runtime.** An uncaught
  throw terminates the title (`libc++abi: terminating due to uncaught exception`)
  even with `"exceptions": true` and the EH runtime linked — the unwinder never
  reaches the landing pad. So `exceptions: true` is *not* yet usable; treat EH as
  broken until this is fixed. (This is exactly the "test it on hardware" caution.)
- **Verified working:** the C11 timed primitives, the `D:\` cwd default, and the
  MSVC `%S`/`%C`/`%I64` translation through `snprintf`/`vsnprintf` — all PASS on
  xemu. The title compile also confirms `-march=pentium3` (no SSE2) and
  `-fno-builtin`.

## Adding a check

Add a `section_*()` to `RxdkTests/main.cpp`, call it from `main()`, and report
with `check(cond, "name")`. Keep the exceptions section last — if EH is broken an
uncaught throw ends the process, so everything else must report first.
