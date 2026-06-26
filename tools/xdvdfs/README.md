# XDVDFS-TR (downloaded host bundle)

Not built in this repo. Install with:

```powershell
.\scripts\install-xdvdfs.ps1
```

Downloads [XDVDFS-TR latest release](https://github.com/Team-Resurgent/XDVDFS-TR/releases/latest) (`xdvdfs-windows-*.zip`) into `tools/xdvdfs/win-x64/`.

Pack a sample XBE as `default.xbe` at the XISO root:

```powershell
.\scripts\Invoke-XbeIsoBuild.ps1 -InputXbe zig-out\xbe\conformance-c.xbe
```

Output defaults to `zig-out/iso/<sample>.iso`.

The zip and extracted tree are local-only (not committed).
