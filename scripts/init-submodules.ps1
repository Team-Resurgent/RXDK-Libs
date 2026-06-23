#Requires -Version 5.1
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path $PSScriptRoot -Parent
Set-Location $RepoRoot

Write-Host 'Initializing submodules...'
git submodule update --init vendor/picolibc vendor/xbox_leak_may_2020 vendor/llvm-project

$llvm = Join-Path $RepoRoot 'vendor/llvm-project'
Write-Host 'Configuring llvm-project sparse checkout (libcxx + libcxxabi only)...'
git -C $llvm sparse-checkout init --cone
git -C $llvm sparse-checkout set libcxx libcxxabi

Write-Host 'Submodule status:'
git submodule status

Write-Host 'llvm-project tree:'
Get-ChildItem $llvm -Name | Where-Object { $_ -notmatch '^\.' }
