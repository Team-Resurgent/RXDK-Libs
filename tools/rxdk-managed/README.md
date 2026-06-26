# RXDK-Tools (downloaded host bundle)

Not built in this repo. Install with:

```powershell
.\scripts\install-rxdk-tools.ps1
```

Downloads [RXDK-Tools latest release](https://github.com/Team-Resurgent/RXDK-Tools/releases/latest) (`rxdk-managed-win-x64.zip`, tag **build-70** at time of writing) into `tools/rxdk-managed/win-x64/`.

Key tool for PE → XBE:

```
tools/rxdk-managed/win-x64/tools/imagebld.exe
```

Convert a sample PE:

```powershell
.\scripts\Invoke-ImageBuild.ps1 -InputExe zig-out\samples\conformance-c\conformance-c.exe -XbeDebug -NoLibWarn
```

Output defaults to `zig-out/xbe/<sample>.xbe`.

The zip and extracted tree are local-only (not committed).
