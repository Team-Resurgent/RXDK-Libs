# Download RXDK-Tools managed host bundle (imagebld, xbox-launch, etc.).
# Release: https://github.com/Team-Resurgent/RXDK-Tools/releases/latest
param(
    [string]$Tag = 'latest',
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$destRoot = Join-Path $root 'tools\rxdk-managed\win-x64'
$imagebld = Join-Path $destRoot 'tools\imagebld.exe'

if ((Test-Path -LiteralPath $imagebld) -and -not $Force) {
    Write-Host "RXDK-Tools already installed: $imagebld"
    exit 0
}

$api = if ($Tag -eq 'latest') {
    'https://api.github.com/repos/Team-Resurgent/RXDK-Tools/releases/latest'
} else {
    "https://api.github.com/repos/Team-Resurgent/RXDK-Tools/releases/tags/$Tag"
}

$release = Invoke-RestMethod -Uri $api -Headers @{ 'User-Agent' = 'RXDK-LibsZig' }
$asset = $release.assets | Where-Object { $_.name -eq 'rxdk-managed-win-x64.zip' } | Select-Object -First 1
if (-not $asset) {
    throw "rxdk-managed-win-x64.zip not found on release $($release.tag_name)"
}

$downloadDir = Join-Path $root 'tools\rxdk-managed'
New-Item -ItemType Directory -Force -Path $downloadDir | Out-Null
$zipPath = Join-Path $downloadDir 'rxdk-managed-win-x64.zip'

Write-Host "Downloading $($release.tag_name): $($asset.browser_download_url)"
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath -UseBasicParsing

$extractRoot = Join-Path $downloadDir '_extract'
if (Test-Path -LiteralPath $extractRoot) {
    Remove-Item -LiteralPath $extractRoot -Recurse -Force
}
Expand-Archive -Path $zipPath -DestinationPath $extractRoot -Force

$extracted = Join-Path $extractRoot 'dist\rxdk-managed-win-x64'
if (-not (Test-Path -LiteralPath $extracted)) {
    throw "Unexpected zip layout (missing dist/rxdk-managed-win-x64)"
}

if (Test-Path -LiteralPath $destRoot) {
    Remove-Item -LiteralPath $destRoot -Recurse -Force
}
Move-Item -LiteralPath $extracted -Destination $destRoot
Remove-Item -LiteralPath $extractRoot -Recurse -Force

Write-Host "Installed: $imagebld"
