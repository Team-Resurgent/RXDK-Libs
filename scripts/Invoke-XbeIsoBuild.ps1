# Pack an XBE into an XISO with default.xbe at the image root (XDVDFS-TR).
param(
    [Parameter(Mandatory)]
    [string]$InputXbe,
    [string]$OutputIso,
    [string]$DefaultName = 'default.xbe'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$xdvdfs = Join-Path $root 'tools\xdvdfs\win-x64\xdvdfs.exe'

if (-not (Test-Path -LiteralPath $xdvdfs)) {
    & (Join-Path $PSScriptRoot 'install-xdvdfs.ps1')
}

$inputFull = [IO.Path]::GetFullPath($InputXbe)
if (-not (Test-Path -LiteralPath $inputFull)) {
    throw "Input XBE not found: $inputFull"
}

if (-not $OutputIso) {
    $outDir = Join-Path $root 'zig-out\iso'
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
    $OutputIso = Join-Path $outDir ([IO.Path]::GetFileNameWithoutExtension($inputFull) + '.iso')
}
$outputFull = [IO.Path]::GetFullPath($OutputIso)
New-Item -ItemType Directory -Force -Path (Split-Path $outputFull -Parent) | Out-Null

$staging = Join-Path ([IO.Path]::GetTempPath()) ("rxdk-xbe-iso-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $staging | Out-Null
try {
    Copy-Item -LiteralPath $inputFull -Destination (Join-Path $staging $DefaultName) -Force

    Write-Host "$xdvdfs pack $staging $outputFull"
    & $xdvdfs pack $staging $outputFull
    if ($LASTEXITCODE -ne 0) {
        throw "xdvdfs pack failed (exit $LASTEXITCODE)"
    }
}
finally {
    Remove-Item -LiteralPath $staging -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Wrote $outputFull ($((Get-Item -LiteralPath $outputFull).Length) bytes)"
return $outputFull
