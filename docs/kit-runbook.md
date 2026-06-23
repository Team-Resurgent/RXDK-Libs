# Kit runbook — RXDK-LibsZig samples

Build PEs in this repo with **`zig build`** only. Deploying to a devkit uses **external** MSBuild/host tools from RXDK-Libs.

## 1. Build PEs

```powershell
cd D:\Git\RXDK-LibsZig
zig build verify-no-vs
zig build kernel-smoke
zig build hello-c
zig build hello-cpp
zig build conformance-c23
zig build conformance-cpp23
```

`compile.ps1` accepts either the zig step name or the artifact folder name:

```powershell
.\scripts\compile.ps1 -Target conformance-c23 -Iso
.\scripts\compile.ps1 -Target c23-stdbit-smoke -Iso
```

Artifacts:

```
zig-out/samples/kernel-smoke/kernel-smoke.exe
zig-out/samples/hello-c/hello-c.exe
zig-out/samples/hello-cpp/hello-cpp.exe
zig-out/samples/c23-stdbit-smoke/c23-stdbit-smoke.exe
zig-out/samples/cpp23-expected-smoke/cpp23-expected-smoke.exe
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
.\scripts\Invoke-ImageBuild.ps1 -InputExe zig-out\samples\kernel-smoke\kernel-smoke.exe -XbeDebug -NoLibWarn
.\scripts\Invoke-ImageBuild.ps1 -InputExe zig-out\samples\hello-c\hello-c.exe -XbeDebug -NoLibWarn
.\scripts\Invoke-ImageBuild.ps1 -InputExe zig-out\samples\hello-cpp\hello-cpp.exe -XbeDebug -NoLibWarn
.\scripts\Invoke-ImageBuild.ps1 -InputExe zig-out\samples\c23-stdbit-smoke\c23-stdbit-smoke.exe -XbeDebug -NoLibWarn
.\scripts\Invoke-ImageBuild.ps1 -InputExe zig-out\samples\cpp23-expected-smoke\cpp23-expected-smoke.exe -XbeDebug -NoLibWarn
```

Or call `imagebld` directly (after `.\scripts\Patch-PeXbox.ps1 -Path zig-out\samples\hello-c\hello-c.exe`):

```powershell
$ib = tools\rxdk-managed\win-x64\tools\imagebld.exe
& $ib /in:zig-out\samples\hello-c\hello-c.exe /out:zig-out\xbe\hello-c.xbe /nologo /stack:1048576 /debug /nolibwarn /INITFLAGS:0 /DONTMOUNTUD /DONTMODIFYHD
```

Verified locally (build-70): `kernel-smoke.exe` → `kernel-smoke.xbe` (24 KB), `hello-c.exe` → `hello-c.xbe` (528 KB).

## 4. XBE → XISO (`default.xbe`)

Install [XDVDFS-TR](https://github.com/Team-Resurgent/XDVDFS-TR/releases/latest) (Windows CLI):

```powershell
.\scripts\install-xdvdfs.ps1
```

Pack an XBE with `default.xbe` at the image root (required for Xbox boot from disc image):

```powershell
.\scripts\Invoke-XbeIsoBuild.ps1 -InputXbe zig-out\xbe\hello-c.xbe
```

Or build PE, XBE, and ISO in one step:

```powershell
.\scripts\compile.ps1 -Target hello-c -Iso
```

Output: `zig-out/iso/hello-c.iso` containing `/default.xbe`.

Verify contents:

```powershell
tools\xdvdfs\win-x64\xdvdfs.exe tree zig-out\iso\hello-c.iso
```

## 5. Deploy to devkit

Use RXDK-Libs deploy scripts (e.g. `Invoke-XboxDeploy.ps1`, neighborhood) — out of scope for `zig build`.

## 6. Expected debug output

| Sample | Expected `DbgPrint` / debug console line |
|--------|----------------------------------------|
| kernel-smoke | `RXDK-LibsZig kernel-smoke OK` |
| hello-c | `RXDK-LibsZig hello-c OK` |
| hello-cpp | `RXDK-LibsZig hello-cpp OK` |
| c23-stdbit-smoke | `RXDK-LibsZig c23-stdbit-smoke OK` |
| cpp23-expected-smoke | `RXDK-LibsZig cpp23-expected-smoke OK` |

`hello-c` / `hello-cpp` route stdio through `write` → `OutputDebugStringA` → `DbgPrint`.

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
