# Convert a RXDK-Libs sample PE (.exe) to .xbe using RXDK-Tools imagebld.
param(
    [Parameter(Mandatory)]
    [string]$InputExe,
    [string]$OutputXbe,
    [switch]$XbeDebug,
    [switch]$NoLibWarn,
    [switch]$BootDisc,
    [switch]$MountHdd,
    [switch]$FormatHdd,
    [int]$MaxImportThunks = 0,
    # 128 KiB (XDK default is 64 KiB). Generous headroom only -- the original
    # "DmEnetFunc overflows the stack" theory was WRONG (the real bug was the
    # xnet/xbdm CXbdmServer protocol mismatch, now fixed via the CXbdmClient
    # bridge). Can likely return to 65536 once the bridge is HW-validated.
    [int]$StackSize = 131072
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

# No PE pre-patch: imagebld (Rxdk.XbeImage) coerces the subsystem to Xbox and
# resolves the real TLS directory itself. The main-thread stack comes from the XBE
# header SizeOfStackCommit, which imagebld sets from /stack (passed below); the XBE
# startup spawns main on a thread sized from it.

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
        # XINIT_FORMAT_UTILITY_DRIVE (0x02) reformats the utility drive at init.
        $initFlags = 1
        if ($FormatHdd) { $initFlags = $initFlags -bor 2 }
        $args += "/INITFLAGS:$initFlags"
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
