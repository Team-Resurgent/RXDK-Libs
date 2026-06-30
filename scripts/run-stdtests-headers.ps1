#Requires -Version 5.1
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path $PSScriptRoot -Parent
Set-Location $RepoRoot

Write-Host '==> python tools/check_c_headers.py' -ForegroundColor Cyan
python (Join-Path $RepoRoot 'tools\check_c_headers.py') @args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
