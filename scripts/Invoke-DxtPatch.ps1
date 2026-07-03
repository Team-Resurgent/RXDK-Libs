# Patch a linked DXT (debug-monitor extension) PE with RXDK-Tools imagebld /DXT:
# coerces the PE subsystem to IMAGE_SUBSYSTEM_XBOX (14) so it matches a retail
# .dxt. No XBE wrapping -- xbdm loads the raw PE from E:\dxt. In-place if no
# -OutputDxt is given.
param(
    [Parameter(Mandatory)]
    [string]$InputDxt,
    [string]$OutputDxt
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$imagebld = Join-Path $root 'tools\rxdk-managed\win-x64\tools\imagebld.exe'

if (-not (Test-Path -LiteralPath $imagebld)) {
    & (Join-Path $PSScriptRoot 'install-rxdk-tools.ps1')
}

$inputFull = [IO.Path]::GetFullPath($InputDxt)
if (-not (Test-Path -LiteralPath $inputFull)) {
    throw "Input DXT not found: $inputFull"
}

$imgArgs = @('/DXT', $inputFull)
if ($OutputDxt) { $imgArgs += [IO.Path]::GetFullPath($OutputDxt) }

Write-Host "$imagebld $($imgArgs -join ' ')"
& $imagebld @imgArgs
if ($LASTEXITCODE -ne 0) {
    throw "imagebld /DXT failed (exit $LASTEXITCODE)"
}
