#Requires -Version 5.1
param(
    [string]$Root = "",
    [string]$RxdkLibsRoot = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $Root) { $Root = Join-Path $PSScriptRoot '..' }
if (-not $RxdkLibsRoot) { $RxdkLibsRoot = $env:RXDK_LIBS_ROOT }

$Root = [IO.Path]::GetFullPath($Root)
if (-not $RxdkLibsRoot) {
    $RxdkLibsRoot = [IO.Path]::GetFullPath((Join-Path $Root '..\RXDK-LibsOld'))
}
$Vendor = Join-Path $Root 'vendor\xbox_private'
$SrcVendor = Join-Path $RxdkLibsRoot 'vendor'

if (-not (Test-Path (Join-Path $SrcVendor 'private\ntos\xapi'))) {
    throw "RXDK-LibsOld vendor/xapi not found at $SrcVendor. Set RXDK_LIBS_ROOT or clone RXDK-LibsOld beside this repo."
}

function Sync-Tree {
    param([string]$Rel)
    $from = Join-Path $SrcVendor $Rel
    $to = Join-Path $Vendor $Rel
    if (-not (Test-Path -LiteralPath $from)) {
        Write-Warning "Skip missing: $from"
        return
    }
    New-Item -ItemType Directory -Force -Path (Split-Path $to -Parent) | Out-Null
    Write-Host "==> robocopy $Rel"
    robocopy $from $to /MIR /NFL /NDL /NJH /NJS /NC /NS | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE) for $Rel" }
}

Write-Host "Syncing xAPI vendor from $SrcVendor -> $Vendor"

@(
    'private\ntos\xapi',
    'private\ntos\rtl',
    'private\ntos\fatx',
    'private\ntos\inc',
    'private\ntos\idex',
    'private\inc',
    'private\inc\crypto',
    'private\genx\types\uuid'
) | ForEach-Object { Sync-Tree $_ }

@(
    'private\ntos\dd\usb\inc',
    'private\ntos\dd\usb\ohcd',
    'private\ntos\dd\usb\usbd',
    'private\ntos\dd\usb\usbhub',
    'private\ntos\dd\usb\mu',
    'private\ntos\dd\usb\xid',
    'private\ntos\dd\usb\xmouse_dbg',
    'private\ntos\dd\usb\xidex'
) | ForEach-Object { Sync-Tree $_ }

$XdkInc = Join-Path $Root 'include\xdk'
New-Item -ItemType Directory -Force -Path $XdkInc | Out-Null
$fromXdk = Join-Path $SrcVendor 'public\xdk\inc'
if (-not (Test-Path -LiteralPath $fromXdk)) {
    throw "Missing public XDK headers: $fromXdk"
}
Write-Host "==> robocopy public xdk inc -> include/xdk"
robocopy $fromXdk $XdkInc /MIR /NFL /NDL /NJH /NJS /NC /NS | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed for xdk inc" }

$SdkInc = Join-Path $Root 'include\sdk'
New-Item -ItemType Directory -Force -Path $SdkInc | Out-Null
$fromSdk = Join-Path $SrcVendor 'public\sdk\inc'
if (-not (Test-Path -LiteralPath $fromSdk)) {
    throw "Missing public SDK headers: $fromSdk"
}
Write-Host "==> robocopy public sdk inc -> include/sdk"
robocopy $fromSdk $SdkInc /MIR /NFL /NDL /NJH /NJS /NC /NS | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed for sdk inc" }

Write-Host "Done. Vendor at $Vendor, headers at $XdkInc and $SdkInc"
