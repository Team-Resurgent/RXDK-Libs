# Kit runbook — RXDK-Libs samples

Build the libraries with the RXDK engine (`build.ps1` / `rxdk build-sdk`, which drives clang/lld/llvm-ar). Samples + tests build as titles via their `rxdk.project.json` (`rxdk build`). Deploying to a devkit uses **external** host tools (`imagebld`, `xdvdfs`) under `tools/`.

The fastest path is the root menu:

```powershell
.\build.ps1            # pick a sample, build PE → XBE → ISO
```

The rest of this doc is the manual pipeline behind that menu.

## 1. Build PEs

```powershell
cd D:\Git\RXDK-Libs
.uild.ps1              # build the whole SDK dist (engine build-sdk)
# a single lib:  rxdk build-sdk-lib --repo-root . --manifest build\sdk\libc.json --config Release
# the test titles (tests/) build via the engine from their rxdk.project.json
```

Host header matrix (stdtests manifest, compile-only):

```powershell
.\scripts\run-stdtests-headers.ps1
```

See `docs/conformance.md` for adding runtime tests.

Build step name == artifact folder == PE name for every sample:

```powershell
.\scripts\compile.ps1 -Target libc-smoke -Iso
```

Artifacts:

```
build-out/samples/xapi-smoke/xapi-smoke.exe
build-out/samples/libc-smoke/libc-smoke.exe
build-out/samples/libcpp-smoke/libcpp-smoke.exe
```

## 2. Install RXDK-Tools (`imagebld`)

`imagebld.exe` is **not** built in RXDK-Libs. Install the managed Windows bundle from [RXDK-Tools releases](https://github.com/Team-Resurgent/RXDK-Tools/releases/latest):

```powershell
cd D:\Git\RXDK-Libs
.\scripts\install-rxdk-tools.ps1
```

This unpacks to `tools/rxdk-managed/win-x64/tools/imagebld.exe` (and `xbox-launch.exe`, `xbcp.exe`, etc.).

## 3. Post-link — PE → XBE

```powershell
.\scripts\Invoke-ImageBuild.ps1 -InputExe build-out\samples\xapi-smoke\xapi-smoke.exe -XbeDebug -NoLibWarn
.\scripts\Invoke-ImageBuild.ps1 -InputExe build-out\samples\libc-smoke\libc-smoke.exe -XbeDebug -NoLibWarn
.\scripts\Invoke-ImageBuild.ps1 -InputExe build-out\samples\libcpp-smoke\libcpp-smoke.exe -XbeDebug -NoLibWarn
```

Or call `imagebld` directly (it coerces the subsystem to Xbox and resolves TLS, so no PE pre-patch is needed):

```powershell
$ib = tools\rxdk-managed\win-x64\tools\imagebld.exe
& $ib /in:build-out\samples\libc-smoke\libc-smoke.exe /out:build-out\xbe\libc-smoke.xbe /nologo /stack:65536 /debug /nolibwarn /INITFLAGS:24 /DONTMOUNTUD /DONTMODIFYHD
```

`xapi-smoke` targets the HDD utility drive — `compile.ps1 -Target xapi-smoke -Iso` mounts and formats it by default (`-NoHdd` for a plain boot disc).

## 4. XBE → XISO (`default.xbe`)

Install [XDVDFS-TR](https://github.com/Team-Resurgent/XDVDFS-TR/releases/latest) (Windows CLI):

```powershell
.\scripts\install-xdvdfs.ps1
```

Pack an XBE with `default.xbe` at the image root (required for Xbox boot from disc image):

```powershell
.\scripts\Invoke-XbeIsoBuild.ps1 -InputXbe build-out\xbe\libc-smoke.xbe
```

Or build PE, XBE, and ISO in one step:

```powershell
.\scripts\compile.ps1 -Target libc-smoke -Iso
```

Output: `build-out/iso/libc-smoke.iso` containing `/default.xbe`.

Verify contents:

```powershell
tools\xdvdfs\win-x64\xdvdfs.exe tree build-out\iso\libc-smoke.iso
```

## 5. Deploy to devkit

Use RXDK-LibsOld deploy scripts (e.g. `Invoke-XboxDeploy.ps1`, neighborhood) — out of scope for the library build.

## 6. Expected debug output

| Sample | Expected `DbgPrint` / debug console line |
|--------|----------------------------------------|
| xapi-smoke | `passed=27` then `all runnable tests passed` |
| libc-smoke | `RXDK-Libs libc-smoke OK passed=N failed=0 total=N` |
| libcpp-smoke | `RXDK-Libs libcpp-smoke OK` |

Samples route stdio through `write` → `DbgPrint` (direct kernel import).

## 7. Troubleshooting

| Symptom | Check |
|---------|--------|
| No debug output | Kernel import: `--whole-archive prebuilt/xboxkrnl.lib`; verify `DbgPrint` resolves |
| Disc damaged / XBE won't load | Rebuild PE+XBE; PE must have subsystem 14, one `xboxkrnl` import, `.XBLD` section (`prebuilt/xboxkrnl_xbld.obj`); run `Invoke-PeVerify.ps1` |
| Hang at startup | Entry must be `-e start` with `crt0.S` `_start` |
| Link undefined `_write` | HAL must export `write`, not `_write` |
| Huge PE / many undefined at link | Use object `.rsp` from the engine output (same as build graph) |

## 8. CI / non-Windows hosts

The engine build (clang/lld/llvm-ar via `build-sdk`) runs on Linux/macOS/Windows. Kit deploy steps require Windows host tools and hardware.
