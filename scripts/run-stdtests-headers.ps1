#Requires -Version 5.1
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path $PSScriptRoot -Parent
Set-Location $RepoRoot

$StdtestsManifest = Join-Path $RepoRoot 'vendor\stdtests\template\c_header.txt'
if (-not (Test-Path -LiteralPath $StdtestsManifest)) {
    Write-Host 'Initializing vendor/stdtests submodule...'
    git submodule update --init vendor/stdtests
}

Write-Host '==> python tools/check_c_headers.py' -ForegroundColor Cyan
python (Join-Path $RepoRoot 'tools\check_c_headers.py') @args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
