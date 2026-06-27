# Kit runbook — RXDK-LibsZig samples

Build PEs in this repo with **`zig build`** only. Deploying to a devkit uses **external** host tools (`imagebld`, `xdvdfs`) under `tools/`.

The fastest path is the root menu:

```powershell
.\build-iso.ps1            # pick a sample, build PE → XBE → ISO
```

The rest of this doc is the manual pipeline behind that menu.

## 1. Build PEs

```powershell
cd D:\Git\RXDK-LibsZig
zig build verify-no-vs
zig build xapi-smoke
zig build libc-smoke
zig build libcpp-smoke
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
zig-out/samples/xapi-smoke/xapi-smoke.exe
zig-out/samples/libc-smoke/libc-smoke.exe
zig-out/samples/libcpp-smoke/libcpp-smoke.exe
```

## 2. Install RXDK-Tools (`imagebld`)

`imagebld.exe` is **not** built in RXDK-LibsZig. Install the managed Windows bundle from [RXDK-Tools releases](https://github.com/Team-Resurgent/RXDK-Tools/releases/latest):

```powershell
cd D:\Git\RXDK-LibsZig
.\scripts\install-rxdk-tools.ps1
```

This unpacks to `tools/rxdk-managed/win-x64/tools/imagebld.exe` (and `xbox-launch.exe`, `xbcp.exe`, etc.).

## 3. Post-link — PE → XBE

```powershell
.\scripts\Invoke-ImageBuild.ps1 -InputExe zig-out\samples\xapi-smoke\xapi-smoke.exe -XbeDebug -NoLibWarn
.\scripts\Invoke-ImageBuild.ps1 -InputExe zig-out\samples\libc-smoke\libc-smoke.exe -XbeDebug -NoLibWarn
.\scripts\Invoke-ImageBuild.ps1 -InputExe zig-out\samples\libcpp-smoke\libcpp-smoke.exe -XbeDebug -NoLibWarn
```

Or call `imagebld` directly (it coerces the subsystem to Xbox and resolves TLS, so no PE pre-patch is needed):

```powershell
$ib = tools\rxdk-managed\win-x64\tools\imagebld.exe
& $ib /in:zig-out\samples\libc-smoke\libc-smoke.exe /out:zig-out\xbe\libc-smoke.xbe /nologo /stack:65536 /debug /nolibwarn /INITFLAGS:24 /DONTMOUNTUD /DONTMODIFYHD
```

`xapi-smoke` targets the HDD utility drive — `compile.ps1 -Target xapi-smoke -Iso` mounts and formats it by default (`-NoHdd` for a plain boot disc).

## 4. XBE → XISO (`default.xbe`)

Install [XDVDFS-TR](https://github.com/Team-Resurgent/XDVDFS-TR/releases/latest) (Windows CLI):

```powershell
.\scripts\install-xdvdfs.ps1
```

Pack an XBE with `default.xbe` at the image root (required for Xbox boot from disc image):

```powershell
.\scripts\Invoke-XbeIsoBuild.ps1 -InputXbe zig-out\xbe\libc-smoke.xbe
```

Or build PE, XBE, and ISO in one step:

```powershell
.\scripts\compile.ps1 -Target libc-smoke -Iso
```

Output: `zig-out/iso/libc-smoke.iso` containing `/default.xbe`.

Verify contents:

```powershell
tools\xdvdfs\win-x64\xdvdfs.exe tree zig-out\iso\libc-smoke.iso
```

## 5. Deploy to devkit

Use RXDK-Libs deploy scripts (e.g. `Invoke-XboxDeploy.ps1`, neighborhood) — out of scope for `zig build`.

## 6. Expected debug output

| Sample | Expected `DbgPrint` / debug console line |
|--------|----------------------------------------|
| xapi-smoke | `passed=27` then `all runnable tests passed` |
| libc-smoke | `RXDK-LibsZig libc-smoke OK passed=N failed=0 total=N` |
| libcpp-smoke | `RXDK-LibsZig libcpp-smoke OK` |

Samples route stdio through `write` → `DbgPrint` (direct kernel import).

## 7. Troubleshooting

| Symptom | Check |
|---------|--------|
| No debug output | Kernel import: `--whole-archive prebuilt/xboxkrnl.lib`; verify `DbgPrint` resolves |
| Disc damaged / XBE won't load | Rebuild PE+XBE; PE must have subsystem 14, one `xboxkrnl` import, `.XBLD` section (`prebuilt/xboxkrnl_xbld.obj`); run `Invoke-PeVerify.ps1` |
| Hang at startup | Entry must be `-e start` with `crt0.S` `_start` |
| Link undefined `_write` | HAL must export `write`, not `_write` |
| Huge PE / many undefined at link | Use object `.rsp` from `zig-out/link/` (same as build graph) |

## 8. CI / non-Windows hosts

`zig build` is designed to run on Linux/macOS/Windows with only Zig installed. Kit deploy steps require Windows host tools and hardware.
