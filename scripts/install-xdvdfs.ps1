# Download XDVDFS-TR Windows CLI (pack XISO images with default.xbe at root).
# Release: https://github.com/Team-Resurgent/XDVDFS-TR/releases/latest
param(
    [string]$Tag = 'latest',
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$destRoot = Join-Path $root 'tools\xdvdfs\win-x64'
$xdvdfs = Join-Path $destRoot 'xdvdfs.exe'

if ((Test-Path -LiteralPath $xdvdfs) -and -not $Force) {
    Write-Host "XDVDFS-TR already installed: $xdvdfs"
    exit 0
}

$api = if ($Tag -eq 'latest') {
    'https://api.github.com/repos/Team-Resurgent/XDVDFS-TR/releases/latest'
} else {
    "https://api.github.com/repos/Team-Resurgent/XDVDFS-TR/releases/tags/$Tag"
}

$release = Invoke-RestMethod -Uri $api -Headers @{ 'User-Agent' = 'RXDK-LibsZig' }
$asset = $release.assets | Where-Object { $_.name -like 'xdvdfs-windows-*.zip' } | Select-Object -First 1
if (-not $asset) {
    throw "xdvdfs-windows-*.zip not found on release $($release.tag_name)"
}

$downloadDir = Join-Path $root 'tools\xdvdfs'
New-Item -ItemType Directory -Force -Path $downloadDir | Out-Null
$zipPath = Join-Path $downloadDir 'xdvdfs-windows.zip'

Write-Host "Downloading $($release.tag_name): $($asset.browser_download_url)"
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath -UseBasicParsing

$extractRoot = Join-Path $downloadDir '_extract'
if (Test-Path -LiteralPath $extractRoot) {
    Remove-Item -LiteralPath $extractRoot -Recurse -Force
}
Expand-Archive -Path $zipPath -DestinationPath $extractRoot -Force

$extracted = Join-Path $extractRoot 'xdvdfs.exe'
if (-not (Test-Path -LiteralPath $extracted)) {
    throw "Unexpected zip layout (missing xdvdfs.exe)"
}

New-Item -ItemType Directory -Force -Path $destRoot | Out-Null
Copy-Item -LiteralPath $extracted -Destination $xdvdfs -Force
Remove-Item -LiteralPath $extractRoot -Recurse -Force

Write-Host "Installed: $xdvdfs"
