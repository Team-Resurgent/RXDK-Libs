# Quick XBE / XISO sanity checks (imagebld dump + optional ISO extract compare).
param(
    [Parameter(Mandatory)]
    [string]$InputXbe,
    [string]$InputIso
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$imagebld = Join-Path $root 'tools\rxdk-managed\win-x64\tools\imagebld.exe'
$xdvdfs = Join-Path $root 'tools\xdvdfs\win-x64\xdvdfs.exe'

$xbeFull = [IO.Path]::GetFullPath($InputXbe)
if (-not (Test-Path -LiteralPath $xbeFull)) {
    throw "XBE not found: $xbeFull"
}

if (-not (Test-Path -LiteralPath $imagebld)) {
    & (Join-Path $PSScriptRoot 'install-rxdk-tools.ps1')
}

Write-Host "==> imagebld /DUMP $xbeFull" -ForegroundColor Cyan
& $imagebld /DUMP $xbeFull
if ($LASTEXITCODE -ne 0) {
    throw "imagebld /DUMP failed (exit $LASTEXITCODE)"
}

if ($InputIso) {
    if (-not (Test-Path -LiteralPath $xdvdfs)) {
        & (Join-Path $PSScriptRoot 'install-xdvdfs.ps1')
    }
    $isoFull = [IO.Path]::GetFullPath($InputIso)
    if (-not (Test-Path -LiteralPath $isoFull)) {
        throw "ISO not found: $isoFull"
    }

    Write-Host "`n==> xdvdfs tree $isoFull" -ForegroundColor Cyan
    & $xdvdfs tree $isoFull
    if ($LASTEXITCODE -ne 0) {
        throw "xdvdfs tree failed (exit $LASTEXITCODE)"
    }

    $extracted = Join-Path ([IO.Path]::GetTempPath()) ("rxdk-xbe-verify-" + [Guid]::NewGuid().ToString('N') + ".xbe")
    try {
        Write-Host "`n==> xdvdfs copy-out default.xbe" -ForegroundColor Cyan
        & $xdvdfs copy-out $isoFull default.xbe $extracted
        if ($LASTEXITCODE -ne 0) {
            throw "xdvdfs copy-out failed (exit $LASTEXITCODE)"
        }

        $srcHash = (Get-FileHash -LiteralPath $xbeFull -Algorithm SHA256).Hash
        $isoHash = (Get-FileHash -LiteralPath $extracted -Algorithm SHA256).Hash
        if ($srcHash -ne $isoHash) {
            throw "ISO default.xbe does not match source XBE (SHA256 mismatch)"
        }
        Write-Host "ISO default.xbe matches source XBE (SHA256 OK)" -ForegroundColor Green
    }
    finally {
        Remove-Item -LiteralPath $extracted -Force -ErrorAction SilentlyContinue
    }
}
