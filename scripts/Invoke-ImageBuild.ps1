# Convert a RXDK-LibsZig sample PE (.exe) to .xbe using RXDK-Tools imagebld.
param(
    [Parameter(Mandatory)]
    [string]$InputExe,
    [string]$OutputXbe,
    [switch]$XbeDebug,
    [switch]$NoLibWarn,
    [switch]$BootDisc,
    [switch]$MountHdd,
    [switch]$SkipPePatch,
    [int]$MaxImportThunks = 0,
    [int]$StackSize = 65536
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$imagebld = Join-Path $root 'tools\rxdk-managed\win-x64\tools\imagebld.exe'

if (-not (Test-Path -LiteralPath $imagebld)) {
    & (Join-Path $PSScriptRoot 'install-rxdk-tools.ps1')
}

$inputFull = [IO.Path]::GetFullPath($InputExe)
if (-not (Test-Path -LiteralPath $inputFull)) {
    throw "Input PE not found: $inputFull"
}

if (-not $SkipPePatch) {
    Write-Host "==> patch PE subsystem (IMAGE_SUBSYSTEM_XBOX)" -ForegroundColor Cyan
    & (Join-Path $PSScriptRoot 'Patch-PeXbox.ps1') -Path $inputFull
}

& (Join-Path $PSScriptRoot 'Invoke-PeVerify.ps1') -InputExe $inputFull -MaxImportThunks $MaxImportThunks

if (-not $OutputXbe) {
    $outDir = Join-Path $root 'zig-out\xbe'
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
    $OutputXbe = Join-Path $outDir ([IO.Path]::GetFileNameWithoutExtension($inputFull) + '.xbe')
}
$outputFull = [IO.Path]::GetFullPath($OutputXbe)
New-Item -ItemType Directory -Force -Path (Split-Path $outputFull -Parent) | Out-Null

$args = @(
    "/in:$inputFull"
    "/out:$outputFull"
    '/nologo'
)
if ($XbeDebug) { $args += '/debug' }
if ($NoLibWarn) { $args += '/nolibwarn' }
if ($BootDisc) {
    if ($MountHdd) {
        # Kit with HDD: validate disk, mount per-title U:/T:, utility cache (Z:).
        # XINIT_MOUNT_UTILITY_DRIVE (0x01); omit NO_SETUP / DONT_MODIFY so XapiInitProcess
        # can set up save-game paths on the utility partition.
        $args += '/INITFLAGS:1'
    } else {
        # Boot-disc smoke: skip HDD setup (0x08) and per-title drive writes (0x10).
        # /DONTMOUNTUD skips utility-drive mount; INITFLAGS must include NO_SETUP_HARD_DISK
        # or XapiInitProcess still validates the HDD partition table.
        $args += '/INITFLAGS:24'
        $args += '/DONTMOUNTUD'
        $args += '/DONTMODIFYHD'
    }
}
# Match TriangleXDK / XDK default imagebld (BuildLog: /stack:65536 /debug /nolibwarn).
$args += "/stack:$StackSize"

Write-Host "$imagebld $($args -join ' ')"
& $imagebld @args
if ($LASTEXITCODE -ne 0) {
    throw "imagebld failed (exit $LASTEXITCODE)"
}

Write-Host "Wrote $outputFull ($((Get-Item -LiteralPath $outputFull).Length) bytes)"
return $outputFull
